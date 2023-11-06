#pragma once

#include "common.h"
#include "Objects.h"
#include "Particles.h"
#include "xoshiro256plusplus.h"

#define GAME_W 1066 // 1422
#define GAME_H 800
#define GAME_FPS 60

#define MAX_ENEMIES 1000
#define MAX_BULLETS 1000
#define MAX_PLR_BULLETS 1000
#define MAX_ALLIES 100

#define MAP_W 10'000.0f
#define MAP_H 10'000.0f

#define DIST_OFFSCREEN 800.0f

enum {
	INPUT_RIGHT = 1,
	INPUT_UP    = 1 << 1,
	INPUT_LEFT  = 1 << 2,
	INPUT_DOWN  = 1 << 3,
	INPUT_FIRE  = 1 << 4,
	INPUT_FOCUS = 1 << 5,
	INPUT_BOOST = 1 << 6
};

struct World;
extern World* world;

struct World {
	Player player;
	Enemy* enemies;
	int enemy_count;
	Bullet* bullets;
	int bullet_count;
	Bullet* p_bullets; // player bullets
	int p_bullet_count;
	Ally* allies;
	int ally_count;

	instance_id next_id;

	Particles particles;

	float camera_x;
	float camera_y;
	float camera_base_x;
	float camera_base_y;
	float camera_scale = 1.0f;
	float camera_scale_target = 1.0f;
	float camera_w = (float) GAME_W;
	float camera_h = (float) GAME_H;
	float camera_left;
	float camera_top;

	u32 input;
	u32 input_press;
	u32 input_release;

	mco_coro* co;
	float coro_timer;
	xoshiro256plusplus rng;
	bool paused;
	int frame;

	float screenshake_intensity;
	float screenshake_time;
	float screenshake_timer;

	float interface_update_timer;
	float interface_x;
	float interface_y;
	SDL_Texture* interface_map_texture;

	bool hide_interface;
	bool show_hitboxes;

	struct {
		int cursor;
	} pause_menu;

	void Init();
	void Quit();

	void Update(float delta);
	void UpdatePlayer(Player* p, float delta);
	void PhysicsUpdate(float delta);
	void player_get_hit(Player* p, float dmg);
	bool enemy_get_hit(Enemy* e, float dmg, float split_dir, bool _play_sound = true);

	void ScreenShake(float x, float y, float intensity, float time, bool always = false);

	void Draw(float delta);
	void DrawUI(float delta);
	void draw_ui(float delta);
	void update_interface(float delta);

	Enemy* CreateEnemy();
	Bullet* CreateBullet();
	Bullet* CreatePlrBullet();

	void DestroyEnemy(instance_id id);
	void DestroyBullet(instance_id id);
	void DestroyPlrBullet(instance_id id);

	void DestroyEnemyByIndex(int enemy_idx);
	void DestroyBulletByIndex(int bullet_idx);
	void DestroyPlrBulletByIndex(int p_bullet_idx);
};

void DrawCircleCamWarped(float x, float y, float radius, SDL_Color color = {255, 255, 255, 255});

void DrawSpriteCamWarped(Sprite* sprite, int frame_index,
						 float x, float y,
						 float angle = 0.0f, float xscale = 1.0f, float yscale = 1.0f,
						 SDL_Color color = {255, 255, 255, 255});
