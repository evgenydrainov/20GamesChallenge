#pragma once

#ifdef _WIN32
#include <SDL.h>
#include <SDL_mixer.h>
#else
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#endif

#include "Font.h"
#include "Sprite.h"

#include "xoshiro256plusplus.h"
#include "minicoro.h"

#define GAME_W 1066
#define GAME_H 800
#define GAME_FPS 60
// #define PLAYER_ACC 0.3f
#define PLAYER_ACC 0.5f
#define PLAYER_MAX_SPD 10.0f
#define PLAYER_FOCUS_SPD 3.0f
// #define PLAYER_TURN_SPD 4.0f
#define PLAYER_TURN_SPD 6.0f
#define MAX_ENEMIES 1000
#define MAX_BULLETS 1000
#define MAX_PLR_BULLETS 1000
#define ArrayLength(a) (sizeof(a) / sizeof(*a))
#define MAP_W 10'000.0f
#define MAP_H 10'000.0f
#define ASTEROID_RADIUS_3 50.0f
#define ASTEROID_RADIUS_2 25.0f
#define ASTEROID_RADIUS_1 12.0f

enum {
	INPUT_RIGHT = 1,
	INPUT_UP    = 1 << 1,
	INPUT_LEFT  = 1 << 2,
	INPUT_DOWN  = 1 << 3,
	INPUT_FIRE  = 1 << 4,
	INPUT_FOCUS = 1 << 5,
	INPUT_BOOST = 1 << 6
};

struct Player {
	float x;
	float y;
	float hsp;
	float vsp;
	float radius = 8.0f;
	float dir;
	bool focus;
	float power;
	int power_level = 1;
	float invincibility;
	float health = 100.0f;
	float max_health = 100.0f;
	float fire_timer;
	int fire_queue;
};

struct Enemy {
	float x;
	float y;
	float hsp;
	float vsp;
	float radius = 15.0f;
	int type;
	float health = 50.0f;
	float max_health = 50.0f;
	Sprite* sprite;
	float frame_index;
	float power = 1.0f;
	float angle;
	mco_coro* co;

	union {
		struct { // used by type 10
			float catch_up_timer;
			float acc;
			float max_spd;
		};
	};
};

struct Bullet {
	float x;
	float y;
	float hsp;
	float vsp;
	float radius = 5.0f;
	float dmg = 10.0f;
	float lifespan = 1.5f * 60.0f;
	float lifetime;
};

struct Game {
	Player player;
	Enemy* enemies;
	int enemy_count;
	Bullet* bullets;
	int bullet_count;
	Bullet* p_bullets; // player bullets
	int p_bullet_count;
	float camera_x;
	float camera_y;
	float camera_base_x;
	float camera_base_y;
	float camera_scale = 1.0f;
	unsigned int input;
	unsigned int input_press;
	unsigned int input_release;
	mco_coro* co;
	float time;
	xoshiro256plusplus random;
	float screenshake_intensity;
	float screenshake_time;
	float screenshake_timer;

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

	Mix_Chunk* snd_ship_engine;
	Mix_Chunk* snd_player_shoot;
	Mix_Chunk* snd_shoot;
	Mix_Chunk* snd_hurt;
	Mix_Chunk* snd_explode;
	Mix_Chunk* snd_boss_explode;
	Mix_Chunk* snd_powerup;

	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_Texture* game_texture;
	double prev_time;
	bool quit;
	double fps_sum;
	double fps_timer;
	double fps;
	float interface_update_timer;
	float interface_x;
	float interface_y;
	SDL_Texture* interface_map_texture;
	bool hide_interface;
	bool show_hitboxes;

	void Init();
	void Quit();
	void Run();
	void Frame();

	void update(float delta);
	void update_player(float delta);
	void physics_update(float delta);
	void draw(float delta);
	void draw_ui(float delta);

	Enemy* CreateEnemy();
	Bullet* CreateBullet();
	Bullet* CreatePlrBullet();

	void DestroyEnemy(int enemy_idx);
	void DestroyBullet(int bullet_idx);
	void DestroyPlrBullet(int p_bullet_idx);
};

extern Game* game;
