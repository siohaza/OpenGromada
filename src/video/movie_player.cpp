#include "video/movie_player.h"

#include <algorithm>
#include <bit>
#include <chrono>
#include <climits>
#include <cstring>
#include <deque>
#include <vector>

#ifdef OPENGROMADA_HAVE_FFMPEG
#include <SDL3/SDL.h>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}
#endif

struct MoviePlayer::Impl {
	std::string error;
	bool playing = false;
	int width = 0, height = 0;
	std::vector<uint32_t> pixels;
#ifdef OPENGROMADA_HAVE_FFMPEG
	FILE* file = nullptr;
	AVFormatContext* format = nullptr;
	AVIOContext* io = nullptr;
	AVCodecContext* video = nullptr;
	AVCodecContext* audio = nullptr;
	AVPacket* packet = nullptr;
	AVFrame* frame = nullptr;
	SwsContext* scaler = nullptr;
	SwrContext* resampler = nullptr;
	SDL_AudioStream* output = nullptr;
	bool audioInitialized = false, eof = false, videoDrain = false, audioDrain = false;
	bool opening = true, paused = false, audioFlushed = false;
	bool videoDone = false, audioDone = true, haveVideo = false, haveAudio = false;
	std::deque<AVPacket*> packets;
	size_t packetBytes = 0;
	int videoIndex = -1, audioIndex = -1;
	int64_t size = 0, origin = 0, videoTime = 0, audioTime = 0, lastVideo = 0, audioEnd = 0;
	int64_t step = 33, duration = 0;
	uint64_t start = 0, elapsed = 0;
	uint64_t pauseTime = 0;
	int64_t readBudget = 0;
	std::chrono::steady_clock::time_point deadline;
	std::vector<uint32_t> pendingVideo;
	std::vector<float> pendingAudio;
	static constexpr int MaxPixels = 1920 * 1080;
	static constexpr int AudioRate = 48000, AudioChannels = 2, AudioBytes = AudioRate * AudioChannels * 4;
	static constexpr int64_t MaxTime = 10 * 60 * 1000;

	~Impl() {
		if (output) SDL_DestroyAudioStream(output);
		if (audioInitialized) SDL_QuitSubSystem(SDL_INIT_AUDIO);
		swr_free(&resampler);
		sws_freeContext(scaler);
		av_packet_free(&packet);
		for (AVPacket* queued : packets) av_packet_free(&queued);
		av_frame_free(&frame);
		avcodec_free_context(&video);
		avcodec_free_context(&audio);
		avformat_close_input(&format);
		if (io) { av_freep(&io->buffer); avio_context_free(&io); }
		if (file) std::fclose(file);
	}
	bool Fail(const char* reason) { error = reason; playing = false; return false; }
	static int Read(void* opaque, uint8_t* data, int count) {
		auto& self = *static_cast<Impl*>(opaque);
		if (count <= 0 || count > 4*1024*1024 ||
			(self.opening ? count > self.readBudget || Interrupted(opaque) : self.readBudget < -4*1024*1024)) return AVERROR(EIO);
		self.readBudget -= count;
		const size_t got = std::fread(data, 1, count, self.file);
		return got ? (int)got : std::ferror(self.file) ? AVERROR(EIO) : AVERROR_EOF;
	}
	static int64_t Seek(void* opaque, int64_t offset, int whence) {
		auto& self = *static_cast<Impl*>(opaque);
		if (whence == AVSEEK_SIZE) return self.size;
		whence &= ~AVSEEK_FORCE;
		int64_t base = whence == SEEK_SET ? 0 : whence == SEEK_END ? self.size : whence == SEEK_CUR ? std::ftell(self.file) : -1;
		if (base < 0 || offset < -base || offset > self.size - base) return AVERROR(EINVAL);
		const int64_t position = base + offset;
		return std::fseek(self.file, (long)position, SEEK_SET) ? AVERROR(EIO) : position;
	}
	static int Interrupted(void* opaque) {
		const auto& self = *static_cast<Impl*>(opaque);
		return self.opening && std::chrono::steady_clock::now() > self.deadline;
	}
	static int DenyOpen(AVFormatContext*, AVIOContext**, const char*, int, AVDictionary**) { return AVERROR(EACCES); }
	int64_t Timestamp(AVFrame* value, int index, int64_t fallback) {
		const int64_t stamp = value->best_effort_timestamp;
		if (stamp == AV_NOPTS_VALUE) return fallback;
		const int64_t ms = av_rescale_q(stamp, format->streams[index]->time_base, AVRational{1,1000});
		if (ms < -MaxTime || ms > MaxTime + origin) { Fail("Movie timestamp exceeds supported timeline"); return 0; }
		return std::max<int64_t>(0, ms - origin);
	}
	bool OpenCodec(int index, AVCodecContext*& context) {
		const auto* parameters = format->streams[index]->codecpar;
		if (parameters->extradata_size < 0 || parameters->extradata_size > 1024*1024) return Fail("Movie codec metadata exceeds allocation bounds");
		const AVCodec* codec = avcodec_find_decoder(parameters->codec_id);
		if (!codec || !(context = avcodec_alloc_context3(codec))) return Fail("Movie decoder is unavailable");
		context->max_pixels = MaxPixels;
		context->max_samples = AudioRate * 2;
		context->thread_count = 1;
		if (avcodec_parameters_to_context(context, parameters) < 0 || avcodec_open2(context, codec, nullptr) < 0)
			return Fail("Cannot initialize movie decoder");
		return true;
	}
	bool Open(uint64_t now) {
		if (!file || std::fseek(file,0,SEEK_END)) return Fail("Movie is not a seekable local file");
		size = std::ftell(file);
		if (size < 16 || size > 512LL*1024*1024 || std::fseek(file,0,SEEK_SET)) return Fail("Movie file size is invalid");
		uint8_t signature[16];
		if (std::fread(signature,1,16,file) != 16 || std::fseek(file,0,SEEK_SET)) return Fail("Truncated movie header");
		static constexpr uint8_t asf[16] = {0x30,0x26,0xb2,0x75,0x8e,0x66,0xcf,0x11,0xa6,0xd9,0,0xaa,0,0x62,0xce,0x6c};
		const char* demux = !std::memcmp(signature,asf,16) ? "asf" :
			!std::memcmp(signature,"RIFF",4) && !std::memcmp(signature+8,"AVI ",4) ? "avi" : nullptr;
		if (!demux) return Fail("Only local ASF/WMV and AVI movie containers are supported");
		format = avformat_alloc_context();
		uint8_t* buffer = static_cast<uint8_t*>(av_malloc(32768));
		if (!format || !buffer) { av_free(buffer); return Fail("Cannot allocate movie input"); }
		io = avio_alloc_context(buffer,32768,0,this,Read,nullptr,Seek);
		if (!io) { av_free(buffer); return Fail("Cannot allocate movie IO"); }
		format->pb = io;
		format->flags |= AVFMT_FLAG_CUSTOM_IO;
		format->io_open = DenyOpen;
		format->interrupt_callback = {Interrupted,this};
		format->probesize = 1024*1024;
		format->max_analyze_duration = 2*AV_TIME_BASE;
		format->max_streams = 4;
		format->max_probe_packets = 128;
		readBudget = 8*1024*1024;
		deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
		const AVInputFormat* input = av_find_input_format(demux);
		if (!input || avformat_open_input(&format,nullptr,input,nullptr) < 0)
			return Fail("Invalid or unsupported movie stream");
		if (!format->nb_streams || format->nb_streams > 4) return Fail("Movie stream count exceeds bounds");
		AVDictionary* probeOptions[4] = {};
		bool probeSafe = true;
		for (unsigned i = 0; i < format->nb_streams; ++i) {
			const auto* p = format->streams[i]->codecpar;
			if (p->extradata_size < 0 || p->extradata_size > 1024*1024 ||
				(p->codec_type == AVMEDIA_TYPE_VIDEO && (p->width <= 0 || p->width > 1920 || p->height <= 0 || p->height > 1080)) ||
				(p->codec_type == AVMEDIA_TYPE_AUDIO && (p->sample_rate < 8000 || p->sample_rate > 96000 || p->ch_layout.nb_channels < 1 || p->ch_layout.nb_channels > 2))) probeSafe = false;


			if (av_dict_set_int(&probeOptions[i],"max_pixels",MaxPixels,0) < 0 ||
				av_dict_set_int(&probeOptions[i],"max_samples",AudioRate*2,0) < 0 ||
				av_dict_set(&probeOptions[i],"threads","1",0) < 0) probeSafe = false;
		}
		const int probed = probeSafe ? avformat_find_stream_info(format,probeOptions) : AVERROR(EINVAL);
		for (auto& options : probeOptions) av_dict_free(&options);
		if (probed < 0) return Fail("Invalid or excessive movie stream metadata");
		videoIndex = av_find_best_stream(format,AVMEDIA_TYPE_VIDEO,-1,-1,nullptr,0);
		audioIndex = av_find_best_stream(format,AVMEDIA_TYPE_AUDIO,-1,-1,nullptr,0);
		if (videoIndex < 0) return Fail("Movie contains no video stream");
		const AVCodecParameters* vp = format->streams[videoIndex]->codecpar;
		if (vp->width <= 0 || vp->height <= 0 || vp->width > 1920 || vp->height > 1080 ||
			int64_t(vp->width)*vp->height > MaxPixels) return Fail("Movie dimensions exceed supported bounds");
		width = vp->width; height = vp->height;
		if (!OpenCodec(videoIndex,video)) return false;
		if (audioIndex >= 0) {
			const AVCodecParameters* ap = format->streams[audioIndex]->codecpar;
			if (ap->sample_rate < 8000 || ap->sample_rate > 96000 || ap->ch_layout.nb_channels < 1 || ap->ch_layout.nb_channels > 2)
				return Fail("Movie audio format exceeds supported bounds");
			if (!OpenCodec(audioIndex,audio)) return false;
			AVChannelLayout inputLayout{}, outputLayout = AV_CHANNEL_LAYOUT_STEREO;
			if (audio->ch_layout.order == AV_CHANNEL_ORDER_UNSPEC) av_channel_layout_default(&inputLayout,audio->ch_layout.nb_channels);
			else if (av_channel_layout_copy(&inputLayout,&audio->ch_layout) < 0) return Fail("Invalid movie audio channel layout");
			int result = swr_alloc_set_opts2(&resampler,&outputLayout,AV_SAMPLE_FMT_FLT,AudioRate,&inputLayout,audio->sample_fmt,audio->sample_rate,0,nullptr);
			av_channel_layout_uninit(&inputLayout);
			if (result < 0 || swr_init(resampler) < 0) return Fail("Cannot initialize movie audio conversion");
			if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) return Fail("Cannot initialize movie audio device");
			audioInitialized = true;
			SDL_AudioSpec spec{SDL_AUDIO_F32,AudioChannels,AudioRate};
			output = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,&spec,nullptr,nullptr);
			if (!output || !SDL_ResumeAudioStreamDevice(output)) return Fail("Cannot open movie audio device");
			audioDone = false;
		}
		origin = format->start_time == AV_NOPTS_VALUE ? 0 : format->start_time / 1000;
		duration = format->duration == AV_NOPTS_VALUE ? 0 : format->duration / 1000;
		if (origin < -MaxTime || origin > MaxTime || duration < 0 || duration > MaxTime) return Fail("Movie duration exceeds supported timeline");
		AVRational rate = av_guess_frame_rate(format,format->streams[videoIndex],nullptr);
		if (rate.num > 0 && rate.den > 0) step = std::clamp<int64_t>(av_rescale(1000,rate.den,rate.num),1,1000);
		packet = av_packet_alloc(); frame = av_frame_alloc();
		if (!packet || !frame) return Fail("Cannot allocate movie frames");
		start = now; opening = false; playing = true;
		return true;
	}
	bool Receive(AVCodecContext* codec, bool isVideo) {
		const int result = avcodec_receive_frame(codec,frame);
		if (result == AVERROR(EAGAIN)) return false;
		if (result == AVERROR_EOF) {
			(isVideo ? videoDone : audioDone) = true;
			if (!isVideo) {
				const int capacity = swr_get_out_samples(resampler,0);
				if (capacity < 0 || capacity > AudioRate*2) { Fail("Movie audio drain exceeds allocation bounds"); return false; }
				pendingAudio.resize(size_t(capacity)*AudioChannels);
				uint8_t* samples[] = {reinterpret_cast<uint8_t*>(pendingAudio.data())};
				const int count = swr_convert(resampler,samples,capacity,nullptr,0);
				if (count < 0) { Fail("Movie audio drain failed"); return false; }
				pendingAudio.resize(size_t(count)*AudioChannels);
				audioTime = audioEnd; audioEnd += int64_t(count)*1000/AudioRate;
				haveAudio = count != 0;
				return haveAudio;
			}
			return false;
		}
		if (result < 0) { Fail("Movie frame decoding failed"); return false; }
		if (isVideo) {
			if (frame->width != width || frame->height != height) { Fail("Changing movie dimensions are unsupported"); return false; }
			pendingVideo.resize(size_t(width)*height);
			const AVPixelFormat pixelFormat = std::endian::native == std::endian::little ? AV_PIX_FMT_BGRA : AV_PIX_FMT_ARGB;
			scaler = sws_getCachedContext(scaler,width,height,static_cast<AVPixelFormat>(frame->format),width,height,pixelFormat,SWS_BILINEAR,nullptr,nullptr,nullptr);
			uint8_t* destinations[4] = {reinterpret_cast<uint8_t*>(pendingVideo.data()),nullptr,nullptr,nullptr};
			int pitches[4] = {width*4,0,0,0};
			if (!scaler || sws_scale(scaler,frame->data,frame->linesize,0,height,destinations,pitches) != height) {
				Fail("Movie pixel conversion failed"); return false;
			}
			videoTime = Timestamp(frame,videoIndex,lastVideo);
			lastVideo = videoTime + step; haveVideo = true;
		}
		else {
			if (frame->sample_rate != audio->sample_rate || frame->ch_layout.nb_channels != audio->ch_layout.nb_channels ||
				frame->format != audio->sample_fmt || frame->nb_samples < 0 || frame->nb_samples > AudioRate*2) {
				Fail("Changing or excessive movie audio frames are unsupported"); return false;
			}
			const int capacity = swr_get_out_samples(resampler,frame->nb_samples);
			if (capacity < 0 || capacity > AudioRate*2) { Fail("Movie audio conversion exceeds queue bounds"); return false; }
			pendingAudio.resize(size_t(capacity)*AudioChannels);
			uint8_t* samples[] = {reinterpret_cast<uint8_t*>(pendingAudio.data())};
			const int count = swr_convert(resampler,samples,capacity,const_cast<const uint8_t**>(frame->extended_data),frame->nb_samples);
			if (count < 0) { Fail("Movie audio conversion failed"); return false; }
			pendingAudio.resize(size_t(count)*AudioChannels);
			audioTime = Timestamp(frame,audioIndex,audioEnd);
			audioEnd = audioTime + (int64_t(count)*1000/AudioRate); haveAudio = count != 0;
		}
		av_frame_unref(frame);
		return true;
	}
	void Update(uint64_t now) {
		if (paused) return;
		elapsed = std::max(elapsed,now >= start ? now-start : uint64_t(0));
		if (elapsed > uint64_t(duration ? duration+5000 : MaxTime+5000)) { Fail("Movie playback exceeded its completion deadline"); return; }
		deadline = std::chrono::steady_clock::now()+std::chrono::milliseconds(8);
		readBudget = 4*1024*1024;
		for (int work = 0; playing && work < 64 && std::chrono::steady_clock::now() < deadline; ++work) {
			if (haveVideo && videoTime <= int64_t(elapsed)) { pixels.swap(pendingVideo); haveVideo = false; }
			if (haveAudio && audioTime <= int64_t(elapsed)+25) {
				if (audioEnd + 250 < int64_t(elapsed)) { SDL_ClearAudioStream(output); haveAudio = false; }
				else if (SDL_GetAudioStreamQueued(output) < AudioBytes/4) {
					if (!SDL_PutAudioStreamData(output,pendingAudio.data(),int(pendingAudio.size()*sizeof(float)))) { Fail("Movie audio queue failed"); break; }
					haveAudio = false;
				}
			}
			if ((haveVideo || videoDone) && (haveAudio || audioDone)) {
				if (haveVideo || haveAudio) break;
			}
			if (!haveVideo && !videoDone && Receive(video,true)) continue;
			if (!playing) break;
			if (audio && !haveAudio && !audioDone && Receive(audio,false)) continue;
			if (!playing) break;



			bool sent = false;
			for (auto it = packets.begin(); it != packets.end(); ++it) {
				AVPacket* queued = *it;
				const bool isVideo = queued->stream_index == videoIndex;
				if (isVideo ? haveVideo : haveAudio) continue;
				const int result = avcodec_send_packet(isVideo ? video : audio,queued);
				if (result == AVERROR(EAGAIN)) continue;
				packetBytes -= queued->size;
				av_packet_free(&queued); packets.erase(it);
				if (result < 0) Fail("Movie packet decoding failed");
				sent = true; break;
			}
			if (sent || !playing) continue;
			if (eof) {
				bool videoQueued = false, audioQueued = false;
				for (const auto* queued : packets) (queued->stream_index == videoIndex ? videoQueued : audioQueued) = true;
				if (!videoDrain && !videoQueued && !haveVideo) {
					int result = avcodec_send_packet(video,nullptr); videoDrain = result >= 0 || result == AVERROR_EOF;
					if (!videoDrain && result != AVERROR(EAGAIN)) { Fail("Movie video drain failed"); break; }
				}
				if (audio && !audioDrain && !audioQueued && !haveAudio) {
					int result = avcodec_send_packet(audio,nullptr); audioDrain = result >= 0 || result == AVERROR_EOF;
					if (!audioDrain && result != AVERROR(EAGAIN)) { Fail("Movie audio drain failed"); break; }
				}
				if (output && audioDone && !haveAudio && !audioFlushed) {
					if (!SDL_FlushAudioStream(output)) { Fail("Movie audio device drain failed"); break; }
					audioFlushed = true;
				}
				if (videoDone && audioDone) {
					const int64_t mediaEnd = std::max(lastVideo,audioEnd);
					if (pixels.empty() || (duration && mediaEnd + 1000 < duration)) {
						Fail("Movie ended before its declared media timeline"); break;
					}
					if (int64_t(elapsed) >= mediaEnd && (!output ||
						(SDL_GetAudioStreamQueued(output) == 0 && SDL_GetAudioStreamAvailable(output) == 0))) playing = false;
				}
				break;
			}


			if (readBudget <= 0) break;
			const int result = av_read_frame(format,packet);
			if (result == AVERROR_EOF) { eof = true; continue; }
			if (result < 0 || packet->size < 0 || packet->size > 4*1024*1024) { Fail("Movie packet is malformed or exceeds decode budget"); break; }
			if (packet->stream_index == videoIndex || packet->stream_index == audioIndex) {
				if (packets.size() >= 128 || packetBytes + packet->size > 8*1024*1024) { Fail("Movie interleave exceeds buffered packet bounds"); break; }
				AVPacket* queued = av_packet_clone(packet);
				if (!queued) { Fail("Cannot allocate buffered movie packet"); break; }
				packets.push_back(queued); packetBytes += packet->size;
			}
			av_packet_unref(packet);
		}
	}
#endif
};

MoviePlayer::MoviePlayer() : m_impl(std::make_unique<Impl>()) {}
MoviePlayer::~MoviePlayer() = default;
bool MoviePlayer::Available() {
#ifdef OPENGROMADA_HAVE_FFMPEG
	return true;
#else
	return false;
#endif
}
bool MoviePlayer::Open(FILE* owned, uint64_t now) {
	Stop();
#ifdef OPENGROMADA_HAVE_FFMPEG
	m_impl->file = owned;
	if (m_impl->Open(now)) return true;
	const std::string error = m_impl->error;
	Stop(); m_impl->error = error; return false;
#else
	if (owned) std::fclose(owned);
	m_impl->error = "Movie playback requires an FFmpeg-enabled build";
	return false;
#endif
}
void MoviePlayer::Update(uint64_t now) {
#ifdef OPENGROMADA_HAVE_FFMPEG
	if (m_impl->playing) m_impl->Update(now);
	if (!m_impl->playing && m_impl->file) {


		auto finished = std::make_unique<Impl>();
		finished->error = m_impl->error;
		finished->width = m_impl->width; finished->height = m_impl->height;
		finished->pixels = std::move(m_impl->pixels);
		m_impl = std::move(finished);
	}
#endif
}
void MoviePlayer::Stop() { m_impl = std::make_unique<Impl>(); }
void MoviePlayer::Pause(uint64_t now) {
#ifdef OPENGROMADA_HAVE_FFMPEG
	if (!m_impl->playing || m_impl->paused) return;
	m_impl->paused = true;
	m_impl->pauseTime = std::max(now,m_impl->start+m_impl->elapsed);
	if (m_impl->output && !SDL_PauseAudioStreamDevice(m_impl->output)) m_impl->Fail("Cannot pause movie audio");
#endif
}
void MoviePlayer::Resume(uint64_t now) {
#ifdef OPENGROMADA_HAVE_FFMPEG
	if (!m_impl->playing || !m_impl->paused) return;
	if (now >= m_impl->pauseTime) m_impl->start = now-(m_impl->pauseTime-m_impl->start);
	m_impl->paused = false;
	if (m_impl->output && !SDL_ResumeAudioStreamDevice(m_impl->output)) m_impl->Fail("Cannot resume movie audio");
#endif
}
bool MoviePlayer::IsPlaying() const { return m_impl->playing; }
const uint32_t* MoviePlayer::Pixels() const { return m_impl->pixels.empty() ? nullptr : m_impl->pixels.data(); }
int MoviePlayer::Width() const { return m_impl->width; }
int MoviePlayer::Height() const { return m_impl->height; }
const std::string& MoviePlayer::Error() const { return m_impl->error; }
