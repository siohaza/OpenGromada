#ifndef PLATFORM_CURSOR_H
#define PLATFORM_CURSOR_H

struct SDL_Cursor;

SDL_Cursor* Platform_LoadCursor(const char* p_path);

SDL_Cursor* Platform_DefaultCursor();

void Platform_SetCursor(SDL_Cursor* p_cursor);

void Platform_FreeCursor(SDL_Cursor* p_cursor);

#endif
