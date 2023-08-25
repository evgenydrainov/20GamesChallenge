#pragma once

#include "Sprite.h"

#ifdef _WIN32
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif

#include "xoshiro256plusplus.h"
#include "minicoro.h"

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
		struct { // TYPE_ENEMY
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

struct World {
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
	bool paused;

	float screenshake_intensity;
	float screenshake_time;
	float screenshake_timer;

	float interface_update_timer;
	float interface_x;
	float interface_y;
	SDL_Texture* interface_map_texture;

	bool hide_interface;
	bool show_hitboxes;
	float coro_timer;

	struct {
		int cursor;
	} pause_menu;

	void Init();
	void Quit();

	void update(float delta);
	void update_player(float delta);
	void physics_update(float delta);
	void player_get_hit(Player* p, float dmg);
	bool enemy_get_hit(Enemy* e, float dmg, float split_dir, bool _play_sound = true);

	void draw(float delta);
	void draw_ui(float delta);

	Enemy* CreateEnemy();
	Bullet* CreateBullet();
	Bullet* CreatePlrBullet();

	void DestroyEnemy(int enemy_idx);
	void DestroyBullet(int bullet_idx);
	void DestroyPlrBullet(int p_bullet_idx);
};
