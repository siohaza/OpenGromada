#pragma once

#include <cstdint>
#include <cstdio>
#include <memory>
#include <string>



class MoviePlayer {
public:
	MoviePlayer();
	~MoviePlayer();
	MoviePlayer(const MoviePlayer&) = delete;
	MoviePlayer& operator=(const MoviePlayer&) = delete;
	static bool Available();
	bool Open(FILE* owned, uint64_t nowMs);
	void Update(uint64_t nowMs);
	void Pause(uint64_t nowMs);
	void Resume(uint64_t nowMs);
	void Stop();
	bool IsPlaying() const;
	const uint32_t* Pixels() const;
	int Width() const;
	int Height() const;
	const std::string& Error() const;

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};
