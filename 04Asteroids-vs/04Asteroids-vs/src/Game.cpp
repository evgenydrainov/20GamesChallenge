#include "Game.h"

#include "Assets.h"
#include "Audio.h"
#include "stb_sprintf.h"
#include "mathh.h"
#include <string.h>

Game* game;

static double GetTime() {
	return double(SDL_GetPerformanceCounter()) / double(SDL_GetPerformanceFrequency());
}

void Game::Init() {
	SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);
	SDL_Log("Starting game...");

	SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "system");

	if (SDL_Init(SDL_INIT_AUDIO
				 | SDL_INIT_VIDEO) != 0) {
		ERROR("Couldn't initialize SDL: %s", SDL_GetError());
	}

	// Mix_Init(0);

	SDL_Rect display;
	SDL_GetDisplayUsableBounds(0, &display);
	int window_w = GAME_W;
	int window_h = GAME_H;
	if (display.w < window_w || display.h < window_h) {
		window_w /= 2;
		window_h /= 2;
	}

	window = SDL_CreateWindow("Asteroids",
							  SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
							  window_w, window_h,
							  SDL_WINDOW_RESIZABLE);
	SDL_SetWindowMinimumSize(window, GAME_W / 2, GAME_H / 2);

	renderer = SDL_CreateRenderer(window, -1, 0);
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

	SDL_RendererInfo info;
	SDL_GetRendererInfo(renderer, &info);
	if (strcmp(info.name, "direct3d") == 0) {
		SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");
		SDL_Log("Using SDL_VIDEO_MINIMIZE_ON_FOCUS_LOSS=0 workaround...");
	}

	Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 2048);
	Mix_Volume(-1, int(0.25f * float(MIX_MAX_VOLUME)));

	if (!load_all_assets()) {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION,
								 "[INFO]",
								 "An error occured while loading game assets.",
								 nullptr);
	}

	state = GameState::PLAYING;
	world = &world_instance;
	world->Init();

	prev_time = GetTime();
}

void Game::Quit() {
	switch (state) {
		case GameState::PLAYING: {
			world->Quit();
			break;
		}
	}

	free_all_assets();

	if (game_texture) SDL_DestroyTexture(game_texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	// Mix_Quit();
	SDL_Quit();

	SDL_Log("Game finished.");
}

void Game::Run() {
	while (!quit) {
		Frame();
	}
}

void Game::Frame() {
	double t = GetTime();
	double frame_end_time = t + (1.0 / double(fps_cap));

	double elapsed = t - prev_time;
	fps = 1.0 / elapsed;
	prev_time = t;

	skip_frame = frame_advance;
	memset(key_pressed, 0, sizeof(key_pressed));

	SDL_Event ev;
	while (SDL_PollEvent(&ev)) {
		switch (ev.type) {
			case SDL_QUIT: {
				quit = true;
				break;
			}

			case SDL_KEYDOWN: {
				int scancode = ev.key.keysym.scancode;

				if (0 <= scancode && scancode < ArrayLength(key_pressed)) {
					key_pressed[scancode] = true;
				}

				switch (scancode) {
					case SDL_SCANCODE_F4: {
						if (ev.key.keysym.mod == 0) {
							set_fullscreen(!get_fullscreen());
						}
						break;
					}

					case SDL_SCANCODE_F5: {
						frame_advance = true;
						skip_frame = false;
						break;
					}

					case SDL_SCANCODE_F6: {
						frame_advance = false;
						break;
					}
				}
				break;
			}
		}
	}

	float delta;
	if (get_vsync()) {
		delta = (float) (elapsed * double(GAME_FPS));
		delta = clamp(delta, 0.1f, 10.0f);
	} else {
		delta = float(GAME_FPS) / float(fps_cap);
	}

	Update(delta);
	Draw(delta);

#ifndef __EMSCRIPTEN__
	{
		if (!get_vsync()) {
			t = GetTime();
			double time_left = frame_end_time - t;
			if (time_left > 0.0) {
				SDL_Delay(u32(time_left * (0.95 * 1000.0)));
				while (GetTime() < frame_end_time) {}
			}
		}
	}
#endif

	if (sleep_ms > 0) {
		SDL_Delay(sleep_ms);
		sleep_ms = 0;
	}
}

void Game::Update(float delta) {
	double t = GetTime();

	if (!skip_frame) {
		switch (state) {
			case GameState::PLAYING: {
				world->Update(delta);
				break;
			}
		}
	}

	update_took = 1000.0 * (GetTime() - t);
}

void Game::Draw(float delta) {
	double t = GetTime();

	{
		int window_w;
		int window_h;
		SDL_GetWindowSize(window, &window_w, &window_h);

		// Decide on game texture size.

		// game_texture_w = window_w;
		// game_texture_h = window_h;

		// game_texture_h = GAME_H;
		// game_texture_w = (int) (float(game_texture_h) / float(window_h) * float(window_w));

		game_texture_w = GAME_W;
		game_texture_h = GAME_H;

		camera_base_h = GAME_H;
		camera_base_w = (int) (float(camera_base_h) / float(game_texture_h) * float(game_texture_w));

		int w;
		int h;
		if (game_texture) SDL_QueryTexture(game_texture, nullptr, nullptr, &w, &h);
		if (!game_texture || w != game_texture_w || h != game_texture_h) {
			if (game_texture) SDL_DestroyTexture(game_texture);
			game_texture = SDL_CreateTexture(renderer,
											 SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_TARGET,
											 game_texture_w, game_texture_h);
		}
		SDL_SetTextureScaleMode(game_texture,
								options.bilinear_filter ? SDL_ScaleModeLinear : SDL_ScaleModeNearest);

		ui_w = game_texture_w / ui_scale;
		ui_h = game_texture_h / ui_scale;
	}

	{
		SDL_SetRenderTarget(renderer, nullptr);
		SDL_RenderSetLogicalSize(renderer, ui_w, ui_h);

		int mouse_x;
		int mouse_y;
		u32 mouse = SDL_GetMouseState(&mouse_x, &mouse_y);
		SDL_RenderWindowToLogical(renderer, mouse_x, mouse_y, &ui_mouse_x, &ui_mouse_y);
	}

	{
		SDL_SetRenderTarget(renderer, game_texture);

		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);

		// Render game world.
		SDL_RenderSetLogicalSize(renderer, camera_base_w, camera_base_h);

		switch (state) {
			case GameState::PLAYING: {
				world->Draw(delta);
				break;
			}
		}

		// Render UI.
		SDL_RenderSetLogicalSize(renderer, ui_w, ui_h);

		switch (state) {
			case GameState::PLAYING: {
				world->DrawUI(delta);
				break;
			}
		}

		// Draw FPS.
		{
			char buf[32];
			if (get_vsync()) {
				stb_snprintf(buf, sizeof(buf), "%.2f V", fps);
			} else {
				stb_snprintf(buf, sizeof(buf), "%.2f", fps);
			}
			DrawText(renderer, fnt_cp437, buf, ui_w, ui_h, HALIGN_RIGHT, VALIGN_BOTTOM);
		}
	}

	SDL_SetRenderTarget(renderer, nullptr);

	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);

	if (options.letterbox) {
		SDL_RenderSetLogicalSize(renderer, game_texture_w, game_texture_h);
	} else {
		SDL_RenderSetLogicalSize(renderer, 0, 0);
	}

	// Render game texture.
	SDL_RenderCopy(renderer, game_texture, nullptr, nullptr);

	SDL_RenderSetLogicalSize(renderer, 0, 0);

	int x = 0;
	int y = 300;
	if (show_debug_info) {
		char buf[100];
		stb_snprintf(buf, sizeof(buf),
					 "update: %.2fms\n"
					 "draw: %.2fms\n",
					 update_took,
					 draw_took);
		y = DrawText(renderer, fnt_mincho, buf, x, y).y;
		if (state == GameState::PLAYING) {
			char buf[100];
			stb_snprintf(buf, sizeof(buf),
						 "enemies: %d (%d all)\n"
						 "bullets: %d\n"
						 "player bullets: %d\n"
						 "allies: %d\n"
						 "chests: %d\n"
						 "particles: %d\n",
						 world->get_enemy_count(), world->enemy_count,
						 world->bullet_count,
						 world->p_bullet_count,
						 world->ally_count,
						 world->chest_count,
						 world->particles.particle_count);
			y = DrawText(renderer, fnt_mincho, buf, x, y).y;
			if (world->ai_points) {
				char buf[50];
				stb_snprintf(buf, sizeof buf,
							 "ai points: %d\n"
							 "ai wait time: %d\n"
							 "ai wait timer: %d\n",
							 *world->ai_points,
							 *world->ai_tick_wait_time,
							 *world->ai_tick_wait_timer);
				y = DrawText(renderer, fnt_mincho, buf, x, y).y;
			}
		}
		y += 10;
	}
	if (show_audio_channels) {
		// draw audio channels

		for (int i = 0; i < Mix_AllocateChannels(-1); i++) {
			const char* name = "";
			Mix_Chunk* chunk = Mix_GetChunk(i);
			for (int j = 0; j < SOUND_COUNT; j++) {
				if (chunk == Chunks[j]) {
					name = Chunk_Names[j];
					break;
				}
			}
			char buf[100];
			stb_snprintf(buf, sizeof(buf), "%d %u %d %s\n", i, channel_when_played[i], channel_priority[i], name);
			SDL_Color col = Mix_Playing(i) ? SDL_Color{255, 255, 255, 255} : SDL_Color{128, 128, 128, 255};
			y = DrawText(renderer, fnt_mincho, buf, x, y, 0, 0, col).y;
		}
	}

	draw_took = 1000.0 * (GetTime() - t);

	SDL_RenderPresent(renderer);
}

void Game::sleep(float x, float y, u32 ms) {
	if (!is_on_screen(x, y)) {
		return;
	}
	sleep_ms = max(sleep_ms, ms);
}

void Game::set_audio3d(bool enable) {
	options.audio_3d = enable;
	// @Hack
	int vol = Mix_Volume(0, -1);
	Mix_AllocateChannels(0);
	Mix_AllocateChannels(MIX_CHANNELS);
	Mix_Volume(-1, vol);
	memset(channel_when_played, 0, sizeof(channel_when_played));
	memset(channel_priority, 0, sizeof(channel_priority));
}

void Game::set_vsync(bool enable) {
#ifndef __EMSCRIPTEN__
	SDL_RenderSetVSync(renderer, enable);
#endif
}

bool Game::get_vsync() {
#ifdef __EMSCRIPTEN__
	return true;
#else
	SDL_RendererInfo info;
	SDL_GetRendererInfo(renderer, &info);
	return (info.flags & SDL_RENDERER_PRESENTVSYNC) != 0;
#endif
}

void Game::set_fullscreen(bool enable) {
	if (enable) {
		SDL_DisplayMode mode;
		SDL_GetDesktopDisplayMode(SDL_GetWindowDisplayIndex(window), &mode);
		SDL_SetWindowDisplayMode(window, &mode);
		SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
	} else {
		SDL_SetWindowFullscreen(window, 0);
	}
}

bool Game::get_fullscreen() {
	return (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN) != 0;
}

float point_distance_wrapped(float x1, float y1, float x2, float y2) {
	float dist = INFINITY;
	float d;

	d = point_distance(x1 - MAP_W, y1 - MAP_H, x2, y2); if (d < dist) dist = d;
	d = point_distance(x1,         y1 - MAP_H, x2, y2); if (d < dist) dist = d;
	d = point_distance(x1 + MAP_W, y1 - MAP_H, x2, y2); if (d < dist) dist = d;

	d = point_distance(x1 - MAP_W, y1,         x2, y2); if (d < dist) dist = d;
	d = point_distance(x1,         y1,         x2, y2); if (d < dist) dist = d;
	d = point_distance(x1 + MAP_W, y1,         x2, y2); if (d < dist) dist = d;

	d = point_distance(x1 - MAP_W, y1 + MAP_H, x2, y2); if (d < dist) dist = d;
	d = point_distance(x1,         y1 + MAP_H, x2, y2); if (d < dist) dist = d;
	d = point_distance(x1 + MAP_W, y1 + MAP_H, x2, y2); if (d < dist) dist = d;

	return dist;
}

bool is_on_screen(float x, float y) {
	// float center_x = world->camera_x;
	// float center_y = world->camera_y;
	// float dist = point_distance_wrapped(x, y, center_x, center_y);
	// return dist < DIST_OFFSCREEN;

	x -= world->camera_left;
	y -= world->camera_top;

	auto check = [](float x, float y) {
		float off = 100.0f;
		return (-off <= x && x < world->camera_w + off)
			&& (-off <= y && y < world->camera_h + off);
	};

	bool result = false;

	result |= check(x - MAP_W, y - MAP_H);
	result |= check(x,         y - MAP_H);
	result |= check(x + MAP_W, y - MAP_H);

	result |= check(x - MAP_W, y);
	result |= check(x,         y);
	result |= check(x + MAP_W, y);

	result |= check(x - MAP_W, y + MAP_H);
	result |= check(x,         y + MAP_H);
	result |= check(x + MAP_W, y + MAP_H);

	return result;
}

float circle_vs_circle_wrapped(float x1, float y1, float r1, float x2, float y2, float r2) {
	float d = point_distance_wrapped(x1, y1, x2, y2);
	return d < (r1 + r2);
}

float point_direction_wrapped(float x1, float y1, float x2, float y2) {
	float dist = INFINITY;
	float d;
	float dir = 0.0f;

	d = point_distance(x1 - MAP_W, y1 - MAP_H, x2, y2); if (d < dist) {dist = d; dir = point_direction(x1 - MAP_W, y1 - MAP_H, x2, y2);}
	d = point_distance(x1,         y1 - MAP_H, x2, y2); if (d < dist) {dist = d; dir = point_direction(x1,         y1 - MAP_H, x2, y2);}
	d = point_distance(x1 + MAP_W, y1 - MAP_H, x2, y2); if (d < dist) {dist = d; dir = point_direction(x1 + MAP_W, y1 - MAP_H, x2, y2);}

	d = point_distance(x1 - MAP_W, y1,         x2, y2); if (d < dist) {dist = d; dir = point_direction(x1 - MAP_W, y1,         x2, y2);}
	d = point_distance(x1,         y1,         x2, y2); if (d < dist) {dist = d; dir = point_direction(x1,         y1,         x2, y2);}
	d = point_distance(x1 + MAP_W, y1,         x2, y2); if (d < dist) {dist = d; dir = point_direction(x1 + MAP_W, y1,         x2, y2);}

	d = point_distance(x1 - MAP_W, y1 + MAP_H, x2, y2); if (d < dist) {dist = d; dir = point_direction(x1 - MAP_W, y1 + MAP_H, x2, y2);}
	d = point_distance(x1,         y1 + MAP_H, x2, y2); if (d < dist) {dist = d; dir = point_direction(x1,         y1 + MAP_H, x2, y2);}
	d = point_distance(x1 + MAP_W, y1 + MAP_H, x2, y2); if (d < dist) {dist = d; dir = point_direction(x1 + MAP_W, y1 + MAP_H, x2, y2);}

	return dir;
}
