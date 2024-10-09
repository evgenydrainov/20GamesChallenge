#pragma once

#include "texture.h"
#include "font.h"

#define GAME_W 640
#define GAME_H 480

struct Player {
	vec2 pos;
	vec2 vel;
	float dir;
	float radius = 8;

	float fire_timer;
	int fire_queue;

	bool focus;
};

struct Bullet {
	vec2 pos;
	vec2 vel;
	float radius = 8;

	float lifetime;
	float lifespan = 60;
};

struct Enemy {
	vec2 pos;
	vec2 vel;
	float dir;
	float radius = 8;

	float fire_timer;
	int fire_queue;
};

struct Camera {
	vec2 pos;
	float zoom = 1;
};

struct Game {
	static constexpr size_t MAX_PLAYER_BULLETS = 1'000;
	static constexpr size_t MAX_ENEMIES = 1'000;
	static constexpr size_t MAX_BULLETS = 1'000;

	Player player;
	bump_array<Bullet> p_bullets;
	bump_array<Enemy> enemies;
	bump_array<Bullet> bullets;

	Camera camera;

	Texture player_texture;
	Font ms_gothic;
	Font ms_mincho;

	bool show_hitboxes;
	bool frame_advance;
	bool skip_frame;

	void init();
	void deinit();

	void update(float delta);
	void draw(float delta);
};

extern Game game;
