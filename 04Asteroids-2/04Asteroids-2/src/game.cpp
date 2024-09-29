#include "game.h"

#include "window_creation.h"
#include "batch_renderer.h"

void Game::init() {
	player_texture = load_texture_from_file("textures/player.png");

	player.x = 100;
	player.y = 100;
}

void Game::deinit() {
	
}

#define PLAYER_ACC      0.30f
#define PLAYER_TURN_SPD 7.0f
#define PLAYER_MAX_SPD  6.0f

static void decelerate(float* hsp, float* vsp, float dec, float delta) {
	float len = glm::length(vec2{*hsp, *vsp});
	if (len > dec) {
		*hsp -= dec * (*hsp / len) * delta;
		*vsp -= dec * (*vsp / len) * delta;
	} else {
		*hsp = 0.0f;
		*vsp = 0.0f;
	}
}

static void limit_speed(float* hsp, float* vsp, float max_spd) {
	float len = glm::length(vec2{*hsp, *vsp});
	if (len > max_spd) {
		*hsp = (*hsp / len) * max_spd;
		*vsp = (*vsp / len) * max_spd;
	}
}

void Game::update(float delta) {
	// update player
	{
		float acc = PLAYER_ACC;
		float turn_spd = PLAYER_TURN_SPD;
		float max_spd = PLAYER_MAX_SPD;

		if (is_key_held(SDL_SCANCODE_UP) || is_key_held(SDL_SCANCODE_DOWN)) {
			turn_spd /= 2.0f;
		}

		if (is_key_held(SDL_SCANCODE_RIGHT)) player.dir -= turn_spd * delta;
		if (is_key_held(SDL_SCANCODE_LEFT))  player.dir += turn_spd * delta;

		if (is_key_held(SDL_SCANCODE_UP)) {
			// accelerate
			player.hsp += lengthdir_x(acc, player.dir) * delta;
			player.vsp += lengthdir_y(acc, player.dir) * delta;
		} else {
			float dec = is_key_held(SDL_SCANCODE_DOWN) ? PLAYER_ACC : (PLAYER_ACC / 16.0f);
			decelerate(&player.hsp, &player.vsp, dec, delta);
		}

		limit_speed(&player.hsp, &player.vsp, max_spd);
	}

	// physics update
	{
		player.x += player.hsp * delta;
		player.y += player.vsp * delta;
	}

	// update camera
	{
		float target_x = player.x + lengthdir_x(50, player.dir);
		float target_y = player.y + lengthdir_y(50, player.dir);

		camera_x += player.hsp * delta;
		camera_y += player.vsp * delta;

		Lerp_Delta(&camera_x, target_x, 0.05f, delta);
		Lerp_Delta(&camera_y, target_y, 0.05f, delta);
	}

	if (is_key_pressed(SDL_SCANCODE_F4, false)) {
		set_fullscreen(!is_fullscreen());
	}
}

void Game::draw(float /*delta*/) {
	float camera_w = GAME_W / camera_zoom;
	float camera_h = GAME_H / camera_zoom;

	float camera_left = camera_x - camera_w / 2.0f;
	float camera_top  = camera_y - camera_h / 2.0f;

	break_batch();
	renderer.proj_mat = glm::ortho<float>(0, camera_w, camera_h, 0);
	renderer.view_mat = glm::translate(mat4{1}, vec3{-camera_left, -camera_top, 0});

	{
		const int size = 32;

		int left = (int) floorf(camera_left / size);
		int top  = (int) floorf(camera_top  / size);

		int right  = left + (int) ceilf(camera_w / size);
		int bottom = top  + (int) ceilf(camera_h / size);

		for (int y = top; y <= bottom; y++) {
			for (int x = left; x <= right; x++) {
				vec4 color;
				if ((x + y) % 2 == 0) {
					color = get_color_4bit(0x002f);
				} else {
					color = get_color_4bit(0x001f);
				}
				draw_rectangle({x * (float)size, y * (float)size, size, size}, color);
			}
		}
	}

	draw_texture(player_texture, {}, {player.x, player.y}, {1, 1}, {player_texture.width / 2, player_texture.height / 2}, player.dir);

	// draw gui
	break_batch();
	renderer.proj_mat = glm::ortho<float>(0, GAME_W, GAME_H, 0);
	renderer.view_mat = {1};

	//draw_rectangle({0, 0, GAME_W, GAME_H}, color_blue);
}
