#include "game.h"

#include "batch_renderer.h"
#include "loading_textures.h"
#include "window_creation.h"

static Texture test_texture;
static float x;
static float y;

void init_game() {
	test_texture = load_texture_from_file("lol.png");
}

void deinit_game() {
	
}

void update_game(float delta) {
	const float speed = 5;

	if (is_key_held(SDL_SCANCODE_RIGHT)) {
		x += speed * delta;
	}

	if (is_key_held(SDL_SCANCODE_LEFT)) {
		x -= speed * delta;
	}

	if (is_key_held(SDL_SCANCODE_DOWN)) {
		y += speed * delta;
	}

	if (is_key_held(SDL_SCANCODE_UP)) {
		y -= speed * delta;
	}
}

void draw_game(float delta) {
	draw_texture(test_texture, {}, {x, y});

	draw_circle({GAME_W / 2, GAME_H / 2}, 90, color_red);
}
