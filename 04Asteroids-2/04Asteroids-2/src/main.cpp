#include "common.h"

#include <raylib.h>
#include <rlgl.h>
#include <raymath.h>





#define GAME_W 640
#define GAME_H 480




struct Player {
	float x;
	float y;
	float dir;
	float hsp;
	float vsp;
};

Player player;
float camera_x;
float camera_y;




float length(float x, float y) {
	return sqrtf(x * x + y * y);
}

void decelerate(float& hsp, float& vsp, float dec, float delta) {
	float l = length(hsp, vsp);
	if (l > dec) {
		hsp -= dec * hsp / l * delta;
		vsp -= dec * vsp / l * delta;
	} else {
		hsp = 0.0f;
		vsp = 0.0f;
	}
}

void limit_speed(float& hsp, float& vsp, float max_spd) {
	float spd = length(hsp, vsp);
	if (spd > max_spd) {
		hsp = (hsp / spd) * max_spd;
		vsp = (vsp / spd) * max_spd;
	}
}

void update(float delta) {
	// update player
	{
		float turn_spd = 6;

		if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_DOWN)) {
			turn_spd /= 2.0f;
		}

		if (IsKeyDown(KEY_RIGHT)) player.dir -= turn_spd * delta;
		if (IsKeyDown(KEY_LEFT))  player.dir += turn_spd * delta;
	}

	if (IsKeyDown(KEY_UP)) {
		const float acc = 0.5f;

		// accelerate
		player.hsp += lengthdir_x(acc, player.dir) * delta;
		player.vsp += lengthdir_y(acc, player.dir) * delta;
	} else {
		float dec = IsKeyDown(KEY_DOWN) ? 0.5f : (0.5f / 16.0f);
		decelerate(player.hsp, player.vsp, dec, delta);
	}

	const float max_speed = 10;
	limit_speed(player.hsp, player.vsp, max_speed);

	player.x += player.hsp * delta;
	player.y += player.vsp * delta;

	float target_x = player.x + lengthdir_x(50, player.dir);
	float target_y = player.y + lengthdir_y(50, player.dir);

	camera_x = lerp_delta(camera_x, target_x, 0.1f, delta);
	camera_y = lerp_delta(camera_y, target_y, 0.1f, delta);
}





void draw_checkerboard_pattern_bg() {
	const int tile_size = 32;

	int left   = (int) floorf((camera_x - GAME_W / 2) / (float)tile_size);
	int top    = (int) floorf((camera_y - GAME_H / 2) / (float)tile_size);
	int right  = left + (GAME_W + (tile_size - 1)) / tile_size;
	int bottom = top  + (GAME_H + (tile_size - 1)) / tile_size;

	for (int y = top; y <= bottom; y++) {
		for (int x = left; x <= right; x++) {
			if ((x + y) % 2) continue;

			DrawRectangle(x * tile_size, y * tile_size, tile_size, tile_size, {30, 30, 30, 255});
		}
	}
}

void draw(float delta) {
	BeginDrawing();
	{
		ClearBackground(BLACK);

		{
			rlDrawRenderBatchActive();

			Matrix proj = MatrixOrtho(0, GAME_W, GAME_H, 0, -1, 1);
			rlSetMatrixProjection(proj);

			int window_w = GetScreenWidth();
			int window_h = GetScreenHeight();

			float xscale = window_w / (float)GAME_W;
			float yscale = window_h / (float)GAME_H;
			float scale = min(xscale, yscale);

			int w = (int) (GAME_W * scale);
			int h = (int) (GAME_H * scale);
			int x = (window_w - w) / 2;
			int y = (window_h - h) / 2;

			rlViewport(x, y, w, h);
		}

		Camera2D cam = {};
		cam.target = {camera_x, camera_y};
		cam.offset = {GAME_W / 2, GAME_H / 2};
		cam.zoom = 1;

		BeginMode2D(cam);
		{
			draw_checkerboard_pattern_bg();

			// draw player
			DrawRectanglePro({player.x, player.y, 16, 16}, {8, 8}, -player.dir, WHITE);
		}
		EndMode2D();
	}
	EndDrawing();
}



int main(int argc, char* argv[]) {
	SetConfigFlags(FLAG_VSYNC_HINT
				   | FLAG_WINDOW_RESIZABLE);

	InitWindow(GAME_W * 2, GAME_H * 2, "Asteroids");
	defer { CloseWindow(); };

	while (!WindowShouldClose()) {
		float delta = GetFrameTime() * 60.0f;

		const float min_fps = 30;
		const float max_fps = 1000;
		Clamp(&delta, 60.0f / max_fps, 60.0f / min_fps);

		update(delta);

		draw(delta);
	}

	return 0;
}
