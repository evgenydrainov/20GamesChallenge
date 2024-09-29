#pragma once

#include "common.h"

struct Window {
	static constexpr int NUM_KEYS = SDL_SCANCODE_UP + 1;

	SDL_Window* handle;
	SDL_GLContext gl_context;
	bool should_quit;
	double target_fps;
	bool vsync;
	int game_width;
	int game_height;

	float fps;
	float delta;

	u32 key_pressed[(NUM_KEYS + 31) / 32];
	u32 key_repeat [(NUM_KEYS + 31) / 32];
	double prev_time;
	double frame_end_time;
};

extern Window window;

void init_window_and_opengl(const char* title,
							int width, int height, int init_scale,
							bool vsync, double target_fps);

void deinit_window_and_opengl();

void handle_event(SDL_Event* ev);

void begin_frame();
void swap_buffers();

bool is_key_held(SDL_Scancode key);
bool is_key_pressed(SDL_Scancode key, bool repeat = true);

SDL_Window* get_window_handle(); // for common.h

void set_fullscreen(bool fullscreen);
bool is_fullscreen();

double get_time();
