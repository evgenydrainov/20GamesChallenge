#include "game.h"

#include "loading_textures.h"
#include "window_creation.h"

void Game::init() {
	player_texture = load_texture_from_file("textures/player.png");

	player.x = 100;
	player.y = 100;
}

void Game::deinit() {
	
}

#define PLAYER_ACC      0.20f
#define PLAYER_TURN_SPD 6.0f
#define PLAYER_MAX_SPD  5.0f

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

		Lerp_Delta(&camera_x, target_x, 0.1f, delta);
		Lerp_Delta(&camera_y, target_y, 0.1f, delta);
	}
}

void Game::draw(float delta) {
	float camera_w = GAME_W / camera_zoom;
	float camera_h = GAME_H / camera_zoom;

	break_batch();
	renderer.proj_mat = glm::ortho<float>(0, camera_w, camera_h, 0);
	renderer.view_mat = glm::translate(mat4{1}, vec3{-(camera_x - camera_w / 2.0f), -(camera_y - camera_h / 2.0f), 0});

	draw_texture(player_texture, {}, {player.x, player.y}, {1, 1}, {player_texture.width / 2, player_texture.height / 2}, player.dir);

	// draw gui
	break_batch();
	renderer.proj_mat = glm::ortho<float>(0, GAME_W, GAME_H, 0);
	renderer.view_mat = {1};

	//draw_rectangle({0, 0, GAME_W, GAME_H}, color_blue);
}
