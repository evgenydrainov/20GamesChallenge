#pragma once

#include "common.h"
#include "World.h"

struct Game;
extern Game* game;

enum struct GameState {
	PLAYING
};

struct Options {
	bool bilinear_filter = true;
	bool letterbox = true;
	bool audio_3d;
};

struct Game {
	union {
		World world_instance{};
	};

	GameState state;
	Options options;

	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_Texture* game_texture;
	bool quit;
	double prev_time;
	double fps;
	double update_took;
	double draw_took;
	bool frame_advance;
	bool skip_frame;
	bool key_pressed[SDL_SCANCODE_UP + 1];
	bool show_debug_info;
	bool show_audio_channels;
	int fps_cap = 60;
	int ui_w = GAME_W;
	int ui_h = GAME_H;
	int ui_scale = 1;
	float ui_mouse_x;
	float ui_mouse_y;
	int camera_base_w = GAME_W;
	int camera_base_h = GAME_H;
	int game_texture_w = GAME_W;
	int game_texture_h = GAME_H;
	u32 sleep_ms;

	void Init();
	void Quit();
	void Run();
	void Frame();

	void Update(float delta);
	void Draw(float delta);

	void sleep(float x, float y, u32 ms);

	void set_audio3d(bool enable);
	void set_vsync(bool enable);
	bool get_vsync();
	void set_fullscreen(bool enable);
	bool get_fullscreen();
};

float point_distance_wrapped(float x1, float y1, float x2, float y2);

bool is_on_screen(float x, float y);

float circle_vs_circle_wrapped(float x1, float y1, float r1, float x2, float y2, float r2);

float point_direction_wrapped(float x1, float y1, float x2, float y2);
