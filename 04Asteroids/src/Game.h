#pragma once

#include "World.h"

#include "Font.h"

#ifdef _WIN32
#include <SDL_mixer.h>
#else
#include <SDL2/SDL_mixer.h>
#endif

#define GAME_W 1066
#define GAME_H 800
#define GAME_FPS 60

#define MAP_W 10'000.0f
#define MAP_H 10'000.0f

#define MY_MIX_CHANNELS 8

enum {
	STATE_PLAYING
};

struct Game {
	union {
		World world_instance;
	};

	int state = STATE_PLAYING;
	bool audio_3d = true;
	float time;

	SDL_Texture* tex_bg;
	SDL_Texture* tex_bg1;
	SDL_Texture* tex_moon;

	Sprite spr_player_ship;
	Sprite spr_asteroid1;
	Sprite spr_asteroid2;
	Sprite spr_asteroid3;
	Sprite spr_invader;

	SpriteGroup sprite_group;

	Font fnt_mincho;

	union {
		struct {
			Mix_Chunk* snd_ship_engine;
			Mix_Chunk* snd_player_shoot;
			Mix_Chunk* snd_shoot;
			Mix_Chunk* snd_boss_shoot;
			Mix_Chunk* snd_hurt;
			Mix_Chunk* snd_explode;
			Mix_Chunk* snd_boss_explode;
			Mix_Chunk* snd_powerup;
		};
		Mix_Chunk* chunks[8];
	};

	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_Texture* game_texture;
	double prev_time;
	bool quit;
	double fps_sum;
	double fps_timer;
	double fps;
	bool hide_interface;
	bool show_hitboxes;
	bool show_audio_channels = true;
	bool show_debug_info = true;
	double update_took, draw_took;

	void Init();
	void Quit();
	void Run();
	void Frame();

	void update(float delta);
	void draw(float delta);
};

extern Game* game;
extern World* world;
extern float channel_when_played[MY_MIX_CHANNELS];
extern int channel_priority[MY_MIX_CHANNELS];
