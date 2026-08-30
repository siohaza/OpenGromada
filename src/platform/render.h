#ifndef PLATFORM_RENDER_H
#define PLATFORM_RENDER_H

struct SDL_Window;
struct SDL_Renderer;

int Platform_RenderOpen(
	const char* p_title,
	int p_outputWidth,
	int p_outputHeight,
	int p_frameWidth,
	int p_frameHeight,
	int p_fullscreen,
	int p_fitAutomaticWindow = 0,
	unsigned int p_display = 0
);

void Platform_RenderClose();

unsigned int* Platform_RenderPixels();

int Platform_RenderPitch();

int Platform_RenderWidth();
int Platform_RenderHeight();

int Platform_RenderResizeLogical(int p_width, int p_height);

void Platform_RenderPresent();

int Platform_RenderHandleDeviceReset();

void Platform_RenderSetFullscreen(int p_fullscreen);
void Platform_RenderSetVSync(int p_vsync);

SDL_Window* Platform_RenderWindow();
SDL_Renderer* Platform_RenderRenderer();

void Platform_RenderRestoreWindowPosition(int p_x, int p_y);

void Platform_RenderWindowToFrame(float* p_x, float* p_y);

void Platform_WarpMouse(float p_x, float p_y);

void Platform_RenderDebugText(float p_x, float p_y, unsigned int p_argb, const char* p_text, int p_height);

#endif
