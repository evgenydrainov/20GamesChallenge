#pragma once

#include "common.h"

extern SDL_Window* window;
extern SDL_GLContext gl_context;
extern bool should_quit;

void init_window_and_opengl(const char* title,
							int width, int height, int init_scale,
							bool vsync);

void deinit_window_and_opengl();

void handle_event(SDL_Event* ev);

void swap_buffers();

bool is_key_held(SDL_Scancode key);
bool is_key_pressed(SDL_Scancode key, bool repeat = true);
