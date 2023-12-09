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
#define MAX_CHESTS 100

#define MAP_W 10'000.0f
#define MAP_H 10'000.0f

#define DIST_OFFSCREEN 800.0f

enum {
	INPUT_RIGHT = 1,
	INPUT_UP    = 1 << 1,
	INPUT_LEFT  = 1 << 2,
	INPUT_DOWN  = 1 << 3,

	INPUT_FIRE     = 1 << 4,
	INPUT_USE_ITEM = 1 << 5,
	INPUT_FOCUS    = 1 << 6,
	INPUT_BOOST    = 1 << 7
};

struct World;
extern World* world;

extern const char* ItemNames[ITEM_COUNT];
extern const char* ItemDescriptions[ITEM_COUNT];
extern const char* ActiveItemNames[ACTIVE_ITEM_COUNT];
extern const char* ActiveItemDescriptions[ACTIVE_ITEM_COUNT];

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
	Chest* chests;
	int chest_count;

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
	xoshiro256plusplus rng_visual;
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

	int* ai_points;
	int* ai_tick_wait_time;
	int* ai_tick_wait_timer;

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

	Enemy*  CreateEnemy();
	Bullet* CreateBullet();
	Bullet* CreatePlrBullet();
	Ally*   CreateAlly();
	Chest*  CreateChest();

	void DestroyEnemyByIndex    (int enemy_idx);
	void DestroyBulletByIndex   (int bullet_idx);
	void DestroyPlrBulletByIndex(int p_bullet_idx);
	void DestroyAllyByIndex     (int ally_idx);
	void DestroyChestByIndex    (int chest_idx);

	int get_enemy_count() {
		int result = 0;
		for (int i = 0; i < enemy_count; i++) {
			if (enemies[i].type >= TYPE_ENEMY) {
				result++;
			}
		}
		return result;
	}
};

void DrawCircleCamWarped(float x, float y, float radius, SDL_Color color = {255, 255, 255, 255});

void DrawSpriteCamWarped(Sprite* sprite, int frame_index,
						 float x, float y,
						 float angle = 0.0f, float xscale = 1.0f, float yscale = 1.0f,
						 SDL_Color color = {255, 255, 255, 255});

SDL_Point DrawTextCamWarped(SDL_Renderer* renderer, Font* font, const char* text,
							int x, int y,
							int halign = 0, int valign = 0,
							SDL_Color color = {255, 255, 255, 255},
							float xscale = 1.0f, float yscale = 1.0f);

SDL_Point DrawTextShadowCamWarped(SDL_Renderer* renderer, Font* font, const char* text,
								  int x, int y,
								  int halign = 0, int valign = 0,
								  SDL_Color color = {255, 255, 255, 255},
								  float xscale = 1.0f, float yscale = 1.0f);
