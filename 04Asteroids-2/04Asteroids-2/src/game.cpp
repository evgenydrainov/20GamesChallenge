#include "game.h"

#include "window_creation.h"
#include "batch_renderer.h"

Game game;

void Game::init() {
	player_texture = load_texture_from_file("textures/player.png");

	player.pos = {100, 100};

	ms_gothic = load_bmfont_file("fonts/ms_gothic.fnt", "fonts/ms_gothic_0.png");
	ms_mincho = load_bmfont_file("fonts/ms_mincho.fnt", "fonts/ms_mincho_0.png");

	p_bullets.data     = (Bullet*) malloc(MAX_PLAYER_BULLETS * sizeof(p_bullets[0]));
	p_bullets.capacity = MAX_PLAYER_BULLETS;

	enemies.data     = (Enemy*) malloc(MAX_ENEMIES * sizeof(enemies[0]));
	enemies.capacity = MAX_ENEMIES;

	bullets.data     = (Bullet*) malloc(MAX_BULLETS * sizeof(bullets[0]));
	bullets.capacity = MAX_BULLETS;

	{
		Enemy e = {};
		array_add(&enemies, e);
	}
}

void Game::deinit() {
	free(bullets.data);
	free(enemies.data);
	free(p_bullets.data);
}

static void accelerate(vec2* vel, float acc, float dir, float delta) {
	vel->x += lengthdir_x(acc, dir) * delta;
	vel->y += lengthdir_y(acc, dir) * delta;
}

static void decelerate(vec2* vel, float dec, float delta) {
	float len = glm::length(*vel);
	if (len > dec) {
		vel->x -= dec * (vel->x / len) * delta;
		vel->y -= dec * (vel->y / len) * delta;
	} else {
		vel->x = 0.0f;
		vel->y = 0.0f;
	}
}

static void limit_speed(vec2* vel, float max_spd) {
	float len = glm::length(*vel);
	if (len > max_spd) {
		vel->x = (vel->x / len) * max_spd;
		vel->y = (vel->y / len) * max_spd;
	}
}

#define PLAYER_ACC       0.30f
#define PLAYER_TURN_SPD  7.0f
#define PLAYER_MAX_SPD   6.0f
#define PLAYER_FOCUS_SPD 2.0f

void Game::update(float delta) {
	skip_frame = frame_advance;

	if (is_key_pressed(SDL_SCANCODE_F4, false)) {
		set_fullscreen(!is_fullscreen());
	}

	if (is_key_pressed(SDL_SCANCODE_H)) {
		show_hitboxes ^= true;
	}

	if (is_key_pressed(SDL_SCANCODE_F5)) {
		frame_advance = true;
		skip_frame = false;
	}

	if (is_key_pressed(SDL_SCANCODE_F6)) {
		frame_advance = false;
	}

	if (skip_frame) {
		return;
	}

	// update player
	{
		player.focus = is_key_held(SDL_SCANCODE_LSHIFT);

		if (player.focus) {
			if (is_key_held(SDL_SCANCODE_RIGHT)) {player.pos.x += PLAYER_FOCUS_SPD * delta; camera.pos.x += PLAYER_FOCUS_SPD * delta;}
			if (is_key_held(SDL_SCANCODE_UP))    {player.pos.y -= PLAYER_FOCUS_SPD * delta; camera.pos.y -= PLAYER_FOCUS_SPD * delta;}
			if (is_key_held(SDL_SCANCODE_LEFT))  {player.pos.x -= PLAYER_FOCUS_SPD * delta; camera.pos.x -= PLAYER_FOCUS_SPD * delta;}
			if (is_key_held(SDL_SCANCODE_DOWN))  {player.pos.y += PLAYER_FOCUS_SPD * delta; camera.pos.y += PLAYER_FOCUS_SPD * delta;}

			decelerate(&player.vel, 2.0f * PLAYER_ACC, delta);
		} else {
			if (is_key_held(SDL_SCANCODE_UP)) {
				accelerate(&player.vel, PLAYER_ACC, player.dir, delta);
			} else {
				float dec = is_key_held(SDL_SCANCODE_DOWN) ? PLAYER_ACC : (PLAYER_ACC / 16.0f);
				decelerate(&player.vel, dec, delta);
			}

			{
				float turn_spd = PLAYER_TURN_SPD;

				if (is_key_held(SDL_SCANCODE_UP) || is_key_held(SDL_SCANCODE_DOWN)) {
					turn_spd /= 2.0f;
				}

				if (is_key_held(SDL_SCANCODE_RIGHT)) player.dir -= turn_spd * delta;
				if (is_key_held(SDL_SCANCODE_LEFT))  player.dir += turn_spd * delta;
			}

			limit_speed(&player.vel, PLAYER_MAX_SPD);
		}

		player.fire_timer += delta;
		while (player.fire_timer >= 10.0f) {
			if (player.fire_queue == 0) {
				if (is_key_held(SDL_SCANCODE_Z)) {
					player.fire_queue = 2;
				}
			}

			if (player.fire_queue > 0) {
				Bullet b = {};
				b.pos = player.pos;

				float spd = 10;
				b.vel = player.vel + lengthdir_v2(spd, player.dir);

				array_add(&p_bullets, b);

				player.fire_queue--;
			}

			player.fire_timer -= 10.0f;
		}
	}

	// update player bullets
	For (b, p_bullets) {
		if (b->lifetime >= b->lifespan) {
			Remove(b, p_bullets);
			continue;
		}

		b->lifetime += delta;
	}

	For (e, enemies) {
		float target_dir = e->dir - angle_difference(e->dir, point_direction(e->pos, player.pos));
		Lerp_delta(&e->dir, target_dir, 0.1f, delta);

		accelerate(&e->vel, 0.30f, e->dir, delta);
		limit_speed(&e->vel, 6.5f);
	}

	// physics update
	{
		player.pos += player.vel * delta;

		For (b, p_bullets) {
			b->pos += b->vel * delta;
		}

		For (e, enemies) {
			e->pos += e->vel * delta;
		}

		For (b, bullets) {
			b->pos += b->vel * delta;
		}
	}

	// update camera
	{
		vec2 target = player.pos + lengthdir_v2(50, player.dir);

		camera.pos += player.vel * delta;

		Lerp_delta(&camera.pos, target, 0.05f, delta);
	}
}

void Game::draw(float /*delta*/) {
	float camera_w = GAME_W / camera.zoom;
	float camera_h = GAME_H / camera.zoom;

	float camera_left = camera.pos.x - camera_w / 2.0f;
	float camera_top  = camera.pos.y - camera_h / 2.0f;

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

	draw_texture_centered(player_texture, player.pos, {1, 1}, player.dir);

	For (b, p_bullets) {
		draw_circle(b->pos, 8, color_white);
	}

	For (e, enemies) {
		draw_texture_centered(player_texture, e->pos, {1, 1}, e->dir);
	}

	For (b, bullets) {
		draw_circle(b->pos, 8, color_white);
	}

	if (show_hitboxes) {
		draw_rectangle({camera_left - 1, camera_top - 1, camera_w + 2, camera_h + 2}, {0, 0, 0, 0.5f});

		draw_circle(player.pos, player.radius, color_white);

		For (b, p_bullets) {
			draw_circle(b->pos, b->radius, color_white);
		}
	}

	// draw gui
	break_batch();
	renderer.proj_mat = glm::ortho<float>(0, GAME_W, GAME_H, 0);
	renderer.view_mat = {1};

	vec2 text_pos = {};

	if (show_hitboxes) {
		text_pos = draw_text(ms_gothic, "H - Show Hitboxes\n", text_pos);
	}
	if (frame_advance) {
		text_pos = draw_text(ms_gothic, "F5 - Next Frame\nF6 - Disable Frame Advance Mode\n", text_pos);
	}
}
