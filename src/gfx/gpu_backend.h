#ifndef GPU_BACKEND_H
#define GPU_BACKEND_H

#include <cstddef>
#include <cstdint>

struct SDL_Renderer;
struct SDL_Texture;

namespace GPU_RENDER
{

struct Command {
	uint32_t op = 0;
	int32_t left = 0;
	int32_t top = 0;
	int32_t right = 0;
	int32_t bottom = 0;
	uint32_t p[59] = {};
};

static_assert(sizeof(Command) == 256);

bool Open(SDL_Renderer* renderer, int width, int height);
void Close();
bool Active();
uint64_t Generation();
bool Resize(int width, int height);
bool Recreate();
bool Flush();
SDL_Texture* OutputTexture(bool update = true);
const char* Error();
void Fail(const char* message);
int Width();
int Height();
uint32_t Color();
uint32_t Depth();
uint32_t Allocate(size_t words);
void Release(uint32_t offset);
uint32_t Upload(const void* identity, const void* data, size_t bytes);
void Forget(const void* identity);
bool Write(uint32_t offset, const void* data, size_t bytes);
bool Read(uint32_t offset, void* data, size_t bytes);
void Submit(const Command& command, bool ordered = false);
bool ReadColor(int x, int y, int width, int height, uint32_t* pixels, int pitch);
uint16_t ReadDepth(int x, int y);
void Clear(uint32_t color, uint16_t depth);

}

#endif
