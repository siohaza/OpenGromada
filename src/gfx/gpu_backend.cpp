#include "gfx/gpu_backend.h"

#include "gfx/gpu_shaders.h"

#include <SDL3/SDL.h>
#include <algorithm>
#include <cstring>
#include <limits>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

namespace GPU_RENDER
{
bool FlushCommands();
namespace
{

constexpr uint32_t TileSize = 8;
constexpr uint32_t MaxCommands = 256;
constexpr uint32_t MaxTileEntries = 1024 * 1024;
constexpr uint32_t MaxHeapWords = 128 * 1024 * 1024;

struct CachedUpload {
	uint32_t offset = 0;
	std::vector<unsigned char> bytes;
};

struct PendingWrite {
	uint32_t offset;
	std::vector<unsigned char> bytes;
};

struct StagingBuffer {
	SDL_GPUTransferBuffer* buffer;
	uint32_t capacity;
	bool busy;
};

struct StagingSubmission {
	SDL_GPUFence* fence;
	std::vector<size_t> buffers;
};

struct BatchUniforms {
	uint32_t count;
	int32_t left;
	int32_t top;
	uint32_t exportColor;
	uint32_t frameWidth;
	uint32_t frameHeight;
	uint32_t colorOffset;
	uint32_t depthOffset;
	uint32_t tilesX;
	uint32_t padding[3];
};

SDL_Renderer* renderer;
SDL_GPUDevice* device;
SDL_GPUComputePipeline* pipelines[501];
SDL_GPUBuffer* heap;
SDL_GPUBuffer* commandBuffer;
SDL_GPUBuffer* tileBuffer;
SDL_GPUTexture* output;
SDL_Texture* texture;
SDL_GPUTransferBuffer* download;
uint32_t downloadCapacity;
uint32_t heapCapacity;
uint32_t heapEnd = 64;
uint32_t color;
uint32_t depth;
int width;
int height;
bool active;
bool dirty;
uint64_t generation;
uint64_t tileEntries;
std::string error;
std::vector<Command> commands;
std::map<uint32_t, uint32_t> allocations;
std::map<uint32_t, uint32_t> freeBlocks;
std::unordered_map<const void*, CachedUpload> uploads;
std::vector<PendingWrite> pendingWrites;
std::vector<uint32_t> retired;
size_t pendingBytes;
SDL_GPUCommandBuffer* recording;
std::vector<size_t> transfers;
std::vector<StagingBuffer> staging;
std::vector<StagingSubmission> stagedSubmissions;
size_t stagingBytes;
size_t recordedBytes;
uint32_t recordedPasses;

bool Check(bool success, const char* operation)
{
	if (!success && error.empty()) {
		error = operation;
		const char* detail = SDL_GetError();
		if (detail && *detail) {
			error += ": ";
			error += detail;
		}
	}
	return success;
}

SDL_GPUBuffer* NewBuffer(uint32_t bytes, SDL_GPUBufferUsageFlags usage)
{
	SDL_GPUBufferCreateInfo info{};
	info.usage = usage;
	info.size = bytes;
	SDL_GPUBuffer* result = SDL_CreateGPUBuffer(device, &info);
	Check(result != nullptr, "Create GPU buffer");
	return result;
}

void ResetRecording()
{
	transfers.clear();
	recordedBytes = 0;
	recordedPasses = 0;
}

bool RetireUploads(bool wait)
{
	for (auto it = stagedSubmissions.begin(); it != stagedSubmissions.end();) {
		if (!SDL_QueryGPUFence(device, it->fence)) {
			if (!wait) {
				++it;
				continue;
			}
			if (!Check(SDL_WaitForGPUFences(device, true, &it->fence, 1), "Wait for GPU staging reuse")) {
				return false;
			}
		}
		for (size_t index : it->buffers) {
			staging[index].busy = false;
		}
		SDL_ReleaseGPUFence(device, it->fence);
		it = stagedSubmissions.erase(it);
	}
	return true;
}

bool SubmitRecording()
{
	if (!recording) {
		return true;
	}
	SDL_GPUCommandBuffer* buffer = recording;
	recording = nullptr;
	bool result;
	if (transfers.empty()) {
		result = Check(SDL_SubmitGPUCommandBuffer(buffer), "Submit GPU command buffer");
	}
	else {
		SDL_GPUFence* fence = SDL_SubmitGPUCommandBufferAndAcquireFence(buffer);
		result = Check(fence != nullptr, "Submit GPU staging command buffer");
		if (fence) {
			stagedSubmissions.push_back({fence, std::move(transfers)});
		}
	}
	ResetRecording();
	return result;
}

size_t AcquireUpload(uint32_t bytes)
{
	if (!bytes || bytes > MaxHeapWords * 4) {
		Fail("GPU staging upload exceeds the memory limit");
		return SIZE_MAX;
	}
	if (!RetireUploads(false)) {
		return SIZE_MAX;
	}
	uint32_t capacity = 4096;
	while (capacity < bytes) {
		capacity *= 2;
	}
	const size_t limit = std::max(size_t(64 * 1024 * 1024), size_t(capacity));
	const auto available = [bytes, limit]() {
		size_t best = SIZE_MAX;
		for (size_t i = 0; i < staging.size(); ++i) {
			if (staging[i].buffer && !staging[i].busy && staging[i].capacity >= bytes && staging[i].capacity <= limit &&
				(best == SIZE_MAX || staging[i].capacity < staging[best].capacity)) {
				best = i;
			}
		}
		return best;
	};
	size_t index = available();
	if (index != SIZE_MAX) {
		staging[index].busy = true;
		return index;
	}
	if (stagingBytes + capacity > limit) {
		if (!SubmitRecording() || !RetireUploads(true)) {
			return SIZE_MAX;
		}
		index = available();
		if (index != SIZE_MAX) {
			staging[index].busy = true;
			return index;
		}
		for (auto& entry : staging) {
			if (entry.buffer && !entry.busy) {
				SDL_ReleaseGPUTransferBuffer(device, entry.buffer);
				stagingBytes -= entry.capacity;
				entry = {};
				if (stagingBytes + capacity <= limit) {
					break;
				}
			}
		}
	}
	SDL_GPUTransferBufferCreateInfo info{};
	info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
	info.size = capacity;
	SDL_GPUTransferBuffer* buffer = SDL_CreateGPUTransferBuffer(device, &info);
	if (!Check(buffer != nullptr, "Create reusable GPU upload buffer")) {
		return SIZE_MAX;
	}
	for (index = 0; index < staging.size() && staging[index].buffer; ++index) {
	}
	if (index == staging.size()) {
		staging.push_back({});
	}
	staging[index] = {buffer, capacity, true};
	stagingBytes += capacity;
	return index;
}

SDL_GPUCommandBuffer* Record()
{
	if ((recordedPasses >= 256 || recordedBytes >= 32 * 1024 * 1024) && !SubmitRecording()) {
		return nullptr;
	}
	if (!recording) {
		recording = SDL_AcquireGPUCommandBuffer(device);
	}
	if (!Check(recording != nullptr, "Acquire GPU command buffer")) {
		return nullptr;
	}
	++recordedPasses;
	return recording;
}

bool ResourceRange(uint32_t offset, size_t bytes)
{
	auto next = allocations.upper_bound(offset);
	if (next == allocations.begin()) {
		return false;
	}
	const auto allocation = std::prev(next);
	const uint64_t end = uint64_t(allocation->first) + allocation->second;
	return offset < end && bytes <= (end - offset) * 4;
}

bool UploadBuffer(SDL_GPUBuffer* destination, uint32_t offset, const void* data, uint32_t bytes)
{
	if (!bytes) {
		return true;
	}
	const size_t index = AcquireUpload(bytes);
	if (index == SIZE_MAX) {
		return false;
	}
	SDL_GPUTransferBuffer* transfer = staging[index].buffer;
	void* mapped = SDL_MapGPUTransferBuffer(device, transfer, false);
	if (!Check(mapped != nullptr, "Map GPU upload buffer")) {
		staging[index].busy = false;
		return false;
	}
	std::memcpy(mapped, data, bytes);
	SDL_UnmapGPUTransferBuffer(device, transfer);
	SDL_GPUCommandBuffer* buffer = Record();
	if (!Check(buffer != nullptr, "Acquire GPU upload command buffer")) {
		staging[index].busy = false;
		return false;
	}
	SDL_GPUCopyPass* pass = SDL_BeginGPUCopyPass(buffer);
	SDL_GPUTransferBufferLocation source{transfer, 0};
	SDL_GPUBufferRegion target{destination, offset, bytes};
	SDL_UploadToGPUBuffer(pass, &source, &target, false);
	SDL_EndGPUCopyPass(pass);
	transfers.push_back(index);
	recordedBytes += bytes;
	return true;
}

bool CommitWrites()
{
	if (pendingWrites.empty()) {
		return true;
	}
	if (pendingBytes > UINT32_MAX) {
		Fail("GPU upload batch is too large");
		return false;
	}
	const size_t index = AcquireUpload(uint32_t(pendingBytes));
	if (index == SIZE_MAX) {
		return false;
	}
	SDL_GPUTransferBuffer* transfer = staging[index].buffer;
	unsigned char* mapped = (unsigned char*) SDL_MapGPUTransferBuffer(device, transfer, false);
	if (!Check(mapped != nullptr, "Map batched GPU upload")) {
		staging[index].busy = false;
		return false;
	}
	uint32_t position = 0;
	for (const auto& write : pendingWrites) {
		std::memcpy(mapped + position, write.bytes.data(), write.bytes.size());
		position += uint32_t(write.bytes.size());
	}
	SDL_UnmapGPUTransferBuffer(device, transfer);
	SDL_GPUCommandBuffer* buffer = Record();
	if (!Check(buffer != nullptr, "Acquire batched GPU upload command buffer")) {
		staging[index].busy = false;
		return false;
	}
	SDL_GPUCopyPass* pass = SDL_BeginGPUCopyPass(buffer);
	position = 0;
	for (const auto& write : pendingWrites) {
		SDL_GPUTransferBufferLocation source{transfer, position};
		SDL_GPUBufferRegion destination{heap, write.offset * 4, uint32_t(write.bytes.size())};
		SDL_UploadToGPUBuffer(pass, &source, &destination, false);
		position += uint32_t(write.bytes.size());
	}
	SDL_EndGPUCopyPass(pass);
	transfers.push_back(index);
	recordedBytes += pendingBytes;
	pendingWrites.clear();
	pendingBytes = 0;
	return true;
}

bool QueueWrite(uint32_t offset, const void* data, size_t bytes)
{
	if (pendingBytes + bytes > 16 * 1024 * 1024 && !FlushCommands()) {
		return false;
	}
	PendingWrite write{};
	write.offset = offset;
	write.bytes.resize((bytes + 3) & ~size_t(3), 0);
	std::memcpy(write.bytes.data(), data, bytes);
	pendingBytes += write.bytes.size();
	pendingWrites.push_back(std::move(write));
	return true;
}

bool GrowHeap(uint32_t required)
{
	if (required <= heapCapacity) {
		return true;
	}
	if (required > MaxHeapWords) {
		Fail("GPU resource memory limit exceeded");
		return false;
	}
	if (!FlushCommands()) {
		return false;
	}
	uint32_t capacity = std::max(1024u * 1024u, heapCapacity);
	while (capacity < required) {
		capacity *= 2;
	}
	SDL_GPUBuffer* replacement =
		NewBuffer(capacity * 4, SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ | SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE);
	if (!replacement) {
		return false;
	}
	if (heap) {
		SDL_GPUCommandBuffer* buffer = Record();
		if (!Check(buffer != nullptr, "Acquire GPU heap copy")) {
			SDL_ReleaseGPUBuffer(device, replacement);
			return false;
		}
		SDL_GPUCopyPass* pass = SDL_BeginGPUCopyPass(buffer);
		SDL_GPUBufferLocation source{heap, 0};
		SDL_GPUBufferLocation destination{replacement, 0};
		SDL_CopyGPUBufferToBuffer(pass, &source, &destination, heapCapacity * 4, false);
		SDL_EndGPUCopyPass(pass);
		SDL_ReleaseGPUBuffer(device, heap);
	}
	heap = replacement;
	heapCapacity = capacity;
	return true;
}

bool CreateOutput(int w, int h, SDL_GPUTexture** native, SDL_Texture** wrapper)
{
	SDL_GPUTextureCreateInfo info{};
	info.type = SDL_GPU_TEXTURETYPE_2D;
	info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
	info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_WRITE;
	info.width = w;
	info.height = h;
	info.layer_count_or_depth = 1;
	info.num_levels = 1;
	info.sample_count = SDL_GPU_SAMPLECOUNT_1;
	*native = SDL_CreateGPUTexture(device, &info);
	if (!Check(*native != nullptr, "Create GPU output texture")) {
		return false;
	}
	SDL_PropertiesID properties = SDL_CreateProperties();
	SDL_SetNumberProperty(properties, SDL_PROP_TEXTURE_CREATE_FORMAT_NUMBER, SDL_PIXELFORMAT_RGBA32);
	SDL_SetNumberProperty(properties, SDL_PROP_TEXTURE_CREATE_ACCESS_NUMBER, SDL_TEXTUREACCESS_STATIC);
	SDL_SetNumberProperty(properties, SDL_PROP_TEXTURE_CREATE_WIDTH_NUMBER, w);
	SDL_SetNumberProperty(properties, SDL_PROP_TEXTURE_CREATE_HEIGHT_NUMBER, h);
	SDL_SetPointerProperty(properties, SDL_PROP_TEXTURE_CREATE_GPU_TEXTURE_POINTER, *native);
	*wrapper = SDL_CreateTextureWithProperties(renderer, properties);
	SDL_DestroyProperties(properties);
	if (!Check(*wrapper != nullptr, "Wrap GPU output texture")) {
		SDL_ReleaseGPUTexture(device, *native);
		*native = nullptr;
		return false;
	}
	SDL_SetTextureScaleMode(*wrapper, SDL_SCALEMODE_NEAREST);
	SDL_SetTextureBlendMode(*wrapper, SDL_BLENDMODE_NONE);
	return true;
}

bool Dispatch(const BatchUniforms& uniforms, uint32_t groupsX, uint32_t groupsY)
{
	const unsigned opcode = uniforms.exportColor ? 0 : commands.empty() ? 501 : commands.front().op;
	if (opcode >= SDL_arraysize(pipelines) || !pipelines[opcode]) {
		Fail("GPU opcode has no compiled pipeline");
		return false;
	}
	SDL_GPUCommandBuffer* buffer = Record();
	if (!Check(buffer != nullptr, "Acquire GPU drawing command buffer")) {
		return false;
	}
	SDL_GPUStorageTextureReadWriteBinding out{};
	out.texture = output;
	SDL_GPUStorageBufferReadWriteBinding storage{};
	storage.buffer = heap;
	SDL_GPUComputePass* pass = SDL_BeginGPUComputePass(buffer,
													   opcode ? nullptr : &out,
													   opcode ? 0 : 1,
													   opcode ? &storage : nullptr,
													   opcode ? 1 : 0);
	SDL_BindGPUComputePipeline(pass, pipelines[opcode]);
	SDL_GPUBuffer* inputs[2] = {opcode ? commandBuffer : heap, tileBuffer};
	SDL_BindGPUComputeStorageBuffers(pass, 0, inputs, opcode ? 2 : 1);
	SDL_PushGPUComputeUniformData(buffer, 0, &uniforms, sizeof(uniforms));
	SDL_DispatchGPUCompute(pass, groupsX, groupsY, 1);
	SDL_EndGPUComputePass(pass);
	return true;
}

bool ExportColor()
{
	if (!dirty) {
		return error.empty() && SubmitRecording();
	}
	if (!FlushCommands()) {
		return false;
	}
	BatchUniforms uniforms{};
	uniforms.exportColor = 1;
	uniforms.frameWidth = width;
	uniforms.frameHeight = height;
	uniforms.colorOffset = color;
	uniforms.depthOffset = depth;
	if (!Dispatch(uniforms, (width + TileSize - 1) / TileSize, (height + TileSize - 1) / TileSize)) {
		return false;
	}
	dirty = false;
	return SubmitRecording();
}

}

bool Open(SDL_Renderer* presentation, int w, int h)
{
	Close();
	error.clear();
	renderer = presentation;
	device = SDL_GetGPURendererDevice(renderer);
	if (!Check(device != nullptr, "Get SDL renderer GPU device")) {
		return false;
	}
	const SDL_GPUShaderFormat formats = SDL_GetGPUShaderFormats(device);
	SDL_GPUComputePipelineCreateInfo info{};
	info.entrypoint = "main";
	if (formats & SDL_GPU_SHADERFORMAT_SPIRV) {
		info.format = SDL_GPU_SHADERFORMAT_SPIRV;
	}
	else if (formats & SDL_GPU_SHADERFORMAT_DXIL) {
		info.format = SDL_GPU_SHADERFORMAT_DXIL;
	}
	else if (formats & SDL_GPU_SHADERFORMAT_MSL) {
		info.format = SDL_GPU_SHADERFORMAT_MSL;
		info.entrypoint = "main0";
	}
	else {
		Fail("No embedded shader format supported by the GPU driver");
		return false;
	}
	info.num_readonly_storage_buffers = 2;
	info.num_readwrite_storage_textures = 1;
	info.num_readwrite_storage_buffers = 1;
	info.num_uniform_buffers = 1;
	info.threadcount_x = TileSize;
	info.threadcount_y = TileSize;
	info.threadcount_z = 1;
	for (const auto& artifact : GPU_SHADERS::artifacts) {
		if (artifact.opcode >= SDL_arraysize(pipelines)) {
			Fail("Compiled GPU opcode exceeds pipeline table");
			return false;
		}
		if (info.format == SDL_GPU_SHADERFORMAT_SPIRV) {
			info.code = artifact.spirv;
			info.code_size = artifact.spirvSize;
		}
		else if (info.format == SDL_GPU_SHADERFORMAT_DXIL) {
			info.code = artifact.dxil;
			info.code_size = artifact.dxilSize;
		}
		else {
			info.code = artifact.msl;
			info.code_size = artifact.mslSize;
		}
		info.num_readonly_storage_buffers = artifact.opcode ? 2 : 1;
		info.num_readwrite_storage_textures = artifact.opcode ? 0 : 1;
		info.num_readwrite_storage_buffers = artifact.opcode ? 1 : 0;
		pipelines[artifact.opcode] = SDL_CreateGPUComputePipeline(device, &info);
		if (!Check(pipelines[artifact.opcode] != nullptr, "Create specialized GPU pipeline")) {
			return false;
		}
	}
	commandBuffer = NewBuffer(MaxCommands * sizeof(Command), SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ);
	tileBuffer = NewBuffer(MaxTileEntries * 8, SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ);
	if (!commandBuffer || !tileBuffer) {
		return false;
	}
	++generation;
	active = true;
	if (!Resize(w, h)) {
		return false;
	}
	Clear(0xff000000u, 0x03ff);
	return Flush() && ExportColor();
}

void Close()
{
	if (device) {
		if (recording) {
			SDL_CancelGPUCommandBuffer(recording);
		}
		recording = nullptr;
		SDL_WaitForGPUIdle(device);
		for (const auto& submission : stagedSubmissions) {
			SDL_ReleaseGPUFence(device, submission.fence);
		}
		for (const auto& entry : staging) {
			if (entry.buffer) {
				SDL_ReleaseGPUTransferBuffer(device, entry.buffer);
			}
		}
		stagedSubmissions.clear();
		staging.clear();
		stagingBytes = 0;
		ResetRecording();
		SDL_DestroyTexture(texture);
		if (output) {
			SDL_ReleaseGPUTexture(device, output);
		}
		if (download) {
			SDL_ReleaseGPUTransferBuffer(device, download);
		}
		if (heap) {
			SDL_ReleaseGPUBuffer(device, heap);
		}
		if (commandBuffer) {
			SDL_ReleaseGPUBuffer(device, commandBuffer);
		}
		if (tileBuffer) {
			SDL_ReleaseGPUBuffer(device, tileBuffer);
		}
		for (auto* pipeline : pipelines) {
			if (pipeline) {
				SDL_ReleaseGPUComputePipeline(device, pipeline);
			}
		}
	}
	renderer = nullptr;
	device = nullptr;
	std::fill_n(pipelines, SDL_arraysize(pipelines), nullptr);
	heap = commandBuffer = tileBuffer = nullptr;
	output = nullptr;
	texture = nullptr;
	download = nullptr;
	downloadCapacity = heapCapacity = color = depth = 0;
	heapEnd = 64;
	width = height = 0;
	active = dirty = false;
	tileEntries = 0;
	commands.clear();
	allocations.clear();
	freeBlocks.clear();
	uploads.clear();
	pendingWrites.clear();
	retired.clear();
	pendingBytes = 0;
}

bool Active()
{
	return active;
}
uint64_t Generation()
{
	return generation;
}
const char* Error()
{
	return error.c_str();
}
int Width()
{
	return width;
}
int Height()
{
	return height;
}
uint32_t Color()
{
	return color;
}
uint32_t Depth()
{
	return depth;
}

void Fail(const char* message)
{
	if (error.empty()) {
		error = message;
	}
}

uint32_t Allocate(size_t words)
{
	if (!active || !error.empty()) {
		return 0;
	}
	if (!words || words > MaxHeapWords) {
		Fail("Invalid GPU resource allocation size");
		return 0;
	}
	words = (words + 3) & ~size_t(3);
	for (auto it = freeBlocks.begin(); it != freeBlocks.end(); ++it) {
		if (it->second >= words) {
			const uint32_t offset = it->first;
			const uint32_t remaining = it->second - (uint32_t) words;
			freeBlocks.erase(it);
			if (remaining) {
				freeBlocks.emplace(offset + (uint32_t) words, remaining);
			}
			allocations.emplace(offset, (uint32_t) words);
			return offset;
		}
	}
	if (words > MaxHeapWords - heapEnd || !GrowHeap(heapEnd + (uint32_t) words)) {
		Fail("GPU resource allocation exceeds available capacity");
		return 0;
	}
	const uint32_t offset = heapEnd;
	heapEnd += (uint32_t) words;
	allocations.emplace(offset, (uint32_t) words);
	return offset;
}

void Release(uint32_t offset)
{
	if (!offset || !FlushCommands()) {
		return;
	}
	auto allocation = allocations.find(offset);
	if (allocation == allocations.end()) {
		return;
	}
	uint32_t size = allocation->second;
	allocations.erase(allocation);
	auto next = freeBlocks.lower_bound(offset);
	if (next != freeBlocks.end() && offset + size == next->first) {
		size += next->second;
		next = freeBlocks.erase(next);
	}
	if (next != freeBlocks.begin()) {
		auto previous = std::prev(next);
		if (previous->first + previous->second == offset) {
			offset = previous->first;
			size += previous->second;
			freeBlocks.erase(previous);
		}
	}
	freeBlocks.emplace(offset, size);
}

bool Write(uint32_t offset, const void* data, size_t bytes)
{
	if (!active || !offset || !data || !ResourceRange(offset, bytes)) {
		Fail("Invalid GPU resource upload range");
		return false;
	}
	if (!FlushCommands()) {
		return false;
	}
	if (offset < color + uint32_t(size_t(width) * height) && uint64_t(offset) + (bytes + 3) / 4 > color) {
		dirty = true;
	}
	return QueueWrite(offset, data, bytes);
}

uint32_t Upload(const void* identity, const void* data, size_t bytes)
{
	if (!identity || !data || !bytes || bytes > size_t(MaxHeapWords) * 4) {
		Fail("Invalid GPU source resource");
		return 0;
	}
	auto& cached = uploads[identity];
	if (cached.offset && cached.bytes.size() == bytes && !std::memcmp(cached.bytes.data(), data, bytes)) {
		return cached.offset;
	}
	const uint32_t replacement = Allocate((bytes + 3) / 4);
	if (!replacement || !QueueWrite(replacement, data, bytes)) {
		return 0;
	}
	if (cached.offset) {
		retired.push_back(cached.offset);
	}
	cached.offset = replacement;
	cached.bytes.assign((const unsigned char*) data, (const unsigned char*) data + bytes);
	return cached.offset;
}

void Forget(const void* identity)
{
	auto it = uploads.find(identity);
	if (it == uploads.end()) {
		return;
	}
	Release(it->second.offset);
	uploads.erase(it);
}

bool Resize(int w, int h)
{
	if (!active || w <= 0 || h <= 0 || size_t(w) * h > MaxHeapWords / 2) {
		Fail("Invalid GPU frame dimensions");
		return false;
	}
	if (w == width && h == height && output) {
		return true;
	}
	if (!FlushCommands()) {
		return false;
	}
	SDL_GPUTexture* native = nullptr;
	SDL_Texture* wrapper = nullptr;
	if (!CreateOutput(w, h, &native, &wrapper)) {
		return false;
	}
	uint32_t newColor = Allocate(size_t(w) * h);
	uint32_t newDepth = Allocate(size_t(w) * h);
	if (!newColor || !newDepth) {
		Release(newColor);
		Release(newDepth);
		SDL_DestroyTexture(wrapper);
		SDL_ReleaseGPUTexture(device, native);
		return false;
	}
	SDL_DestroyTexture(texture);
	if (output) {
		SDL_ReleaseGPUTexture(device, output);
	}
	Release(color);
	Release(depth);
	output = native;
	texture = wrapper;
	color = newColor;
	depth = newDepth;
	width = w;
	height = h;
	dirty = true;
	return true;
}

bool Recreate()
{
	if (!active || !error.empty() || !FlushCommands()) {
		return false;
	}
	SDL_GPUTexture* native = nullptr;
	SDL_Texture* wrapper = nullptr;
	if (!CreateOutput(width, height, &native, &wrapper)) {
		return false;
	}
	SDL_GPUCommandBuffer* buffer = Record();
	if (!buffer) {
		SDL_DestroyTexture(wrapper);
		SDL_ReleaseGPUTexture(device, native);
		return false;
	}
	SDL_GPUCopyPass* pass = SDL_BeginGPUCopyPass(buffer);
	SDL_GPUTextureLocation source{};
	source.texture = output;
	SDL_GPUTextureLocation destination{};
	destination.texture = native;
	SDL_CopyGPUTextureToTexture(pass, &source, &destination, width, height, 1, false);
	SDL_EndGPUCopyPass(pass);
	SDL_DestroyTexture(texture);
	if (output) {
		SDL_ReleaseGPUTexture(device, output);
	}
	output = native;
	texture = wrapper;
	return SubmitRecording();
}

void Submit(const Command& command, bool ordered)
{
	if (!active || !error.empty() || command.right <= command.left || command.bottom <= command.top) {
		return;
	}
	if (command.op != 1 && !(command.op >= 100 && command.op <= 104) && !(command.op >= 200 && command.op <= 201) &&
		!(command.op >= 300 && command.op <= 311) && command.op != 500) {
		Fail("Unsupported GPU drawing operation");
		return;
	}
	const uint64_t w = uint64_t(int64_t(command.right) - command.left);
	const uint64_t h = uint64_t(int64_t(command.bottom) - command.top);
	const uint64_t entries = ((w + 2 * TileSize - 2) / TileSize) * ((h + 2 * TileSize - 2) / TileSize);
	if (w > 65535u * TileSize || h > 65535u * TileSize || entries > MaxTileEntries / 2) {
		Fail("GPU command dimensions exceed tile capacity");
		return;
	}
	if (ordered || (!commands.empty() && commands.back().op != command.op) || commands.size() == MaxCommands ||
		tileEntries + entries > MaxTileEntries / 2) {
		FlushCommands();
	}
	if (!error.empty()) {
		return;
	}
	commands.push_back(command);
	tileEntries += entries;
	dirty = true;
	if (ordered) {
		FlushCommands();
	}
}

bool FlushCommands()
{
	if (!error.empty()) {
		return false;
	}
	if (!CommitWrites()) {
		return false;
	}
	if (commands.empty()) {
		auto released = std::move(retired);
		retired.clear();
		for (uint32_t offset : released) {
			Release(offset);
		}
		return true;
	}
	int left = commands.front().left;
	int top = commands.front().top;
	int right = commands.front().right;
	int bottom = commands.front().bottom;
	for (const auto& command : commands) {
		left = std::min(left, command.left);
		top = std::min(top, command.top);
		right = std::max(right, command.right);
		bottom = std::max(bottom, command.bottom);
	}
	const uint64_t columns = (int64_t(right) - left + TileSize - 1) / TileSize;
	const uint64_t rows = (int64_t(bottom) - top + TileSize - 1) / TileSize;
	if (columns > 65535 || rows > 65535 || columns * rows > MaxTileEntries / 2) {
		auto pending = std::move(commands);
		auto released = std::move(retired);
		commands.clear();
		retired.clear();
		tileEntries = 0;
		for (const auto& command : pending) {
			Submit(command, true);
		}
		for (uint32_t offset : released) {
			Release(offset);
		}
		return error.empty();
	}
	const uint32_t tiles = uint32_t(columns * rows);
	std::vector<uint32_t> counts(tiles, 0);
	for (const auto& command : commands) {
		for (uint32_t y = (command.top - top) / TileSize; y <= (command.bottom - 1 - top) / TileSize; ++y) {
			for (uint32_t x = (command.left - left) / TileSize; x <= (command.right - 1 - left) / TileSize; ++x) {
				++counts[size_t(y) * columns + x];
			}
		}
	}
	uint64_t total = uint64_t(tiles) * 2;
	for (uint32_t count : counts) {
		total += count;
	}
	if (total > MaxTileEntries * 2) {
		Fail("GPU tile index capacity exceeded");
		return false;
	}
	std::vector<uint32_t> indices(size_t(total), 0);
	uint32_t cursor = tiles * 2;
	for (uint32_t tile = 0; tile < tiles; ++tile) {
		indices[tile * 2] = cursor;
		indices[tile * 2 + 1] = counts[tile];
		cursor += counts[tile];
		counts[tile] = 0;
	}
	for (uint32_t index = 0; index < commands.size(); ++index) {
		const Command& command = commands[index];
		for (uint32_t y = (command.top - top) / TileSize; y <= (command.bottom - 1 - top) / TileSize; ++y) {
			for (uint32_t x = (command.left - left) / TileSize; x <= (command.right - 1 - left) / TileSize; ++x) {
				uint32_t tile = uint32_t(y * columns + x);
				indices[indices[tile * 2] + counts[tile]++] = index;
			}
		}
	}
	if (!UploadBuffer(commandBuffer, 0, commands.data(), uint32_t(commands.size() * sizeof(Command))) ||
		!UploadBuffer(tileBuffer, 0, indices.data(), uint32_t(indices.size() * 4))) {
		return false;
	}
	BatchUniforms uniforms{};
	uniforms.count = uint32_t(commands.size());
	uniforms.left = left;
	uniforms.top = top;
	uniforms.frameWidth = width;
	uniforms.frameHeight = height;
	uniforms.colorOffset = color;
	uniforms.depthOffset = depth;
	uniforms.tilesX = uint32_t(columns);
	const bool result = Dispatch(uniforms, uint32_t(columns), uint32_t(rows));
	commands.clear();
	tileEntries = 0;
	auto released = std::move(retired);
	retired.clear();
	for (uint32_t offset : released) {
		Release(offset);
	}
	return result;
}

bool Flush()
{
	return FlushCommands() && SubmitRecording();
}

SDL_Texture* OutputTexture(bool update)
{
	return active && (!update || ExportColor()) ? texture : nullptr;
}

bool Read(uint32_t offset, void* data, size_t bytes)
{
	if (!active || !data || !offset || !ResourceRange(offset, bytes)) {
		Fail("Invalid GPU readback range");
		return false;
	}
	if (!FlushCommands()) {
		return false;
	}
	const uint32_t padded = (uint32_t(bytes) + 3) & ~3u;
	if (padded > downloadCapacity) {
		SDL_GPUTransferBufferCreateInfo info{};
		info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD;
		info.size = std::max(padded, 4096u);
		SDL_GPUTransferBuffer* replacement = SDL_CreateGPUTransferBuffer(device, &info);
		if (!Check(replacement != nullptr, "Create GPU readback buffer")) {
			return false;
		}
		if (download) {
			SDL_ReleaseGPUTransferBuffer(device, download);
		}
		download = replacement;
		downloadCapacity = info.size;
	}
	SDL_GPUCommandBuffer* buffer = Record();
	if (!Check(buffer != nullptr, "Acquire GPU readback command buffer")) {
		return false;
	}
	SDL_GPUCopyPass* pass = SDL_BeginGPUCopyPass(buffer);
	SDL_GPUBufferRegion source{heap, offset * 4, padded};
	SDL_GPUTransferBufferLocation target{download, 0};
	SDL_DownloadFromGPUBuffer(pass, &source, &target);
	SDL_EndGPUCopyPass(pass);
	SDL_GPUFence* fence = SDL_SubmitGPUCommandBufferAndAcquireFence(buffer);
	recording = nullptr;
	if (!Check(fence != nullptr, "Submit GPU readback")) {
		return false;
	}
	const bool waited = Check(SDL_WaitForGPUFences(device, true, &fence, 1), "Wait for GPU readback");
	SDL_ReleaseGPUFence(device, fence);
	if (!waited) {
		return false;
	}
	for (size_t index : transfers) {
		staging[index].busy = false;
	}
	ResetRecording();
	if (!RetireUploads(false)) {
		return false;
	}
	const void* mapped = SDL_MapGPUTransferBuffer(device, download, false);
	if (!Check(mapped != nullptr, "Map GPU readback")) {
		return false;
	}
	std::memcpy(data, mapped, bytes);
	SDL_UnmapGPUTransferBuffer(device, download);
	return true;
}

bool ReadColor(int x, int y, int w, int h, uint32_t* pixels, int pitch)
{
	if (!pixels || x < 0 || y < 0 || w <= 0 || h <= 0 || w > width - x || h > height - y || pitch < w) {
		return false;
	}
	const size_t words = size_t(h - 1) * width + w;
	std::vector<uint32_t> region(words);
	if (!Read(color + uint32_t(size_t(y) * width + x), region.data(), words * 4)) {
		return false;
	}
	for (int row = 0; row < h; ++row) {
		std::memcpy(pixels + size_t(row) * pitch, region.data() + size_t(row) * width, size_t(w) * 4);
	}
	return true;
}

uint16_t ReadDepth(int x, int y)
{
	if (x < 0 || y < 0 || x >= width || y >= height) {
		return 0x03ff;
	}
	uint32_t value = 0x03ff;
	Read(depth + uint32_t(size_t(y) * width + x), &value, sizeof(value));
	return uint16_t(value);
}

void Clear(uint32_t value, uint16_t z)
{
	Command command{};
	command.op = 1;
	command.right = width;
	command.bottom = height;
	command.p[0] = value;
	command.p[1] = z;
	Submit(command);
}

}
