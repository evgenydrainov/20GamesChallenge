#include "common.h"

#include <raylib.h>





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


int main(int argc, char* argv[]) {
	SetConfigFlags(FLAG_VSYNC_HINT
				   | FLAG_WINDOW_RESIZABLE);

	InitWindow(GAME_W, GAME_H, "Asteroids");
	defer { CloseWindow(); };

	while (!WindowShouldClose()) {


		float delta = GetFrameTime() * 60.0f;


		{
			const float min_fps = 30;
			const float max_fps = 1000;

			Clamp(&delta, 60.0f / max_fps, 60.0f / min_fps);
		}



		{
			const float turn_spd = 6;

			if (IsKeyDown(KEY_RIGHT)) player.dir -= turn_spd * delta;
			if (IsKeyDown(KEY_LEFT))  player.dir += turn_spd * delta;


			if (IsKeyDown(KEY_UP)) {
				const float acc = 0.5f;

				player.hsp += acc *  dcos(player.dir) * delta;
				player.vsp += acc * -dsin(player.dir) * delta;
			}

			player.x += player.hsp * delta;
			player.y += player.vsp * delta;
		}


		BeginDrawing();

		ClearBackground(BLACK);

		{
			DrawRectanglePro({player.x, player.y, 16, 16}, {8, 8}, -player.dir, WHITE);
		}

		EndDrawing();
	}

	return 0;
}
