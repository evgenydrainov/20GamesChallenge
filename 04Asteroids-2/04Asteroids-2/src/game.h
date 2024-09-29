#pragma once

#include "texture.h"
#include "font.h"

#define GAME_W 640
#define GAME_H 480

struct Player {
	float x;
	float y;
	float hsp;
	float vsp;
	float dir;
};

struct Game {
	Player player;

	float camera_x;
	float camera_y;
	float camera_zoom = 1;

	Texture player_texture;
	Font ms_gothic;
	Font ms_mincho;

	void init();
	void deinit();

	void update(float delta);
	void draw(float delta);
};
