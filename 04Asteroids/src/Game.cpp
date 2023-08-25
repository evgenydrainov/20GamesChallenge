#include "Game.h"

#ifdef _WIN32
#include <SDL_image.h>
#include <SDL_ttf.h>
#else
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#endif

#include "mathh.h"
#include "misc.h"

#include <stdio.h>

Game* game;
World* world;
float channel_when_played[MY_MIX_CHANNELS];
int channel_priority[MY_MIX_CHANNELS];
static const char* chunk_names[ArrayLength(Game::chunks)];

void Game::Init() {
	SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);

	SDL_Init(SDL_INIT_VIDEO);
	IMG_Init(IMG_INIT_PNG);
	TTF_Init();
	Mix_Init(MIX_INIT_OGG);

	// SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
	// SDL_SetHint(SDL_HINT_RENDER_DRIVER,        "opengl");
	// SDL_SetHint(SDL_HINT_RENDER_BATCHING,      "1");

	window = SDL_CreateWindow("04Asteroids",
							  SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
							  GAME_W, GAME_H,
							  SDL_WINDOW_RESIZABLE);

	renderer = SDL_CreateRenderer(window, -1,
								  SDL_RENDERER_ACCELERATED
								  | SDL_RENDERER_TARGETTEXTURE);

	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

	game_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_TARGET, GAME_W, GAME_H);
	SDL_SetTextureScaleMode(game_texture, bilinear_filter ? SDL_ScaleModeLinear : SDL_ScaleModeNearest);

	tex_bg   = IMG_LoadTexture(renderer, "assets/bg.png"); // https://deep-fold.itch.io/space-background-generator
	tex_bg1  = IMG_LoadTexture(renderer, "assets/bg1.png");
	tex_moon = IMG_LoadTexture(renderer, "assets/moon.png");

	InitSpriteGroup(&sprite_group);

	LoadSprite(&spr_player_ship, &sprite_group, "assets/player_ship.png");
	LoadSprite(&spr_asteroid1,   &sprite_group, "assets/asteroid1.png");
	LoadSprite(&spr_asteroid2,   &sprite_group, "assets/asteroid2.png");
	LoadSprite(&spr_asteroid3,   &sprite_group, "assets/asteroid3.png");
	LoadSprite(&spr_invader,     &sprite_group, "assets/invader.png", 2, 2, 1.0f / 40.0f);

	FinalizeSpriteGroup(&sprite_group);

	LoadFont(&fnt_mincho, "assets/mincho.ttf", 22);

	if (Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 2048) == 0) {
		printf("opened audio device\n");
	} else {
		printf("didn't open audio device\n");
	}

	{
		int i = 0;
		snd_ship_engine  = Mix_LoadWAV("assets/ship_engine.wav");     chunk_names[i++] = "ship_engine.wav";
		snd_player_shoot = Mix_LoadWAV("assets/player_shoot.wav");    chunk_names[i++] = "player_shoot.wav";
		snd_shoot        = Mix_LoadWAV("assets/shoot.wav");           chunk_names[i++] = "shoot.wav";
		snd_boss_shoot   = Mix_LoadWAV("assets/boss_shoot.wav");      chunk_names[i++] = "boss_shoot.wav";
		snd_hurt         = Mix_LoadWAV("assets/hurt.wav");            chunk_names[i++] = "hurt.wav";
		snd_explode      = Mix_LoadWAV("assets/explode.wav");         chunk_names[i++] = "explode.wav";
		snd_boss_explode = Mix_LoadWAV("assets/boss_explode.wav");    chunk_names[i++] = "boss_explode.wav";
		snd_powerup      = Mix_LoadWAV("assets/powerup.wav");         chunk_names[i++] = "powerup.wav";
	}

	Mix_VolumeChunk(snd_ship_engine, (int)(0.5f * (float)MIX_MAX_VOLUME));
	Mix_VolumeChunk(snd_boss_shoot,  (int)(0.5f * (float)MIX_MAX_VOLUME));

	Mix_Volume(-1, (int)(0.25f * (float)MIX_MAX_VOLUME));

	// prev_time = GetTime() - (1.0 / (double)GAME_FPS);

	// printf("emscripten printf\n");
	// SDL_Log("emscripten SDL_Log\n");

	world = &world_instance;
	world->Init();

	// double t = GetTime();
	// for (int i = 10'000; i--;) printf("printf 10k lines\n");
	// printf("%fms", (GetTime() - t) * 1000.0);
}

void Game::Quit() {
	world->Quit();

	Mix_FreeChunk(snd_powerup);
	Mix_FreeChunk(snd_boss_explode);
	Mix_FreeChunk(snd_explode);
	Mix_FreeChunk(snd_hurt);
	Mix_FreeChunk(snd_boss_shoot);
	Mix_FreeChunk(snd_shoot);
	Mix_FreeChunk(snd_player_shoot);
	Mix_FreeChunk(snd_ship_engine);

	DestroyFont(&fnt_mincho);

	DestroySpriteGroup(&sprite_group);

	SDL_DestroyTexture(tex_moon);
	SDL_DestroyTexture(tex_bg1);
	SDL_DestroyTexture(tex_bg);

	SDL_DestroyTexture(game_texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	Mix_Quit();
	TTF_Quit();
	IMG_Quit();
	SDL_Quit();
}

void Game::Run() {
	while (!quit) {
		Frame();
	}
}

void Game::Frame() {
	double t = GetTime();

	double frame_end_time = t + (1.0 / (double)GAME_FPS);

	double current_fps = 1.0 / (t - prev_time);
	prev_time = t;

	skip_frame = frame_advance;
	SDL_memset(&key_pressed, 0, sizeof(key_pressed));

	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
			case SDL_QUIT: {
				quit = true;
				break;
			}

			case SDL_KEYDOWN: {
				int scancode = event.key.keysym.scancode;

				if (0 <= scancode && scancode < ArrayLength(key_pressed)) {
					key_pressed[scancode] = true;
				}

				switch (scancode) {
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

	float delta = 60.0f / float(GAME_FPS);

	fps_sum += current_fps;
	fps_timer += 1.0f;
	if (fps_timer >= float(GAME_FPS)) {
		fps = fps_sum / fps_timer;
		fps_timer = 0.0f;
		fps_sum = 0.0f;
	}

	// fps = current_fps;

	{
		double t = GetTime();

		update(delta);

		update_took = (GetTime() - t) * 1000.0;
	}

	{
		double t = GetTime();

		draw(delta);

		draw_took = (GetTime() - t) * 1000.0;
	}

#ifndef __EMSCRIPTEN__
	t = GetTime();
	double time_left = frame_end_time - t;
	if (time_left > 0.0) {
		SDL_Delay((Uint32) (time_left * (0.95 * 1000.0)));
		while (GetTime() < frame_end_time) {}
	}
#endif
}

void Game::update(float delta) {
	switch (state) {
		case STATE_PLAYING: {
			world->update(delta);
			break;
		}
	}

	time += delta;
}

void Game::draw(float delta) {
	SDL_SetRenderTarget(renderer, game_texture);

	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);

	switch (state) {
		case STATE_PLAYING: {
			world->draw(delta);
			break;
		}
	}

	// draw fps
	{
		char buf[10];
		SDL_snprintf(buf, sizeof(buf), "%.2f", fps);
		DrawText(&fnt_mincho, buf, GAME_W, GAME_H, HALIGN_RIGHT, VALIGN_BOTTOM);
	}

	SDL_SetRenderTarget(renderer, nullptr);

	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);

	if (letterbox) {
		int window_w;
		int window_h;
		SDL_GetWindowSize(window, &window_w, &window_h);
		float scale = min(float(window_w) / float(GAME_W), float(window_h) / float(GAME_H));
		SDL_Rect dest;
		dest.w = (int) (float(GAME_W) * scale);
		dest.h = (int) (float(GAME_H) * scale);
		dest.x = (window_w - dest.w) / 2;
		dest.y = (window_h - dest.h) / 2;
		SDL_RenderCopy(renderer, game_texture, nullptr, &dest);
	} else {
		SDL_RenderCopy(renderer, game_texture, nullptr, nullptr);
	}

	int x = 0;
	int y = 200;
	if (show_debug_info) {
		char buf[100];
		SDL_snprintf(buf, sizeof(buf),
					 "update: %.2fms\n"
					 "draw: %.2fms",
					 update_took,
					 draw_took);
		y = DrawText(&fnt_mincho, buf, x, y).y + fnt_mincho.height + 10;
	}
	if (show_audio_channels) {
		// draw audio channels

		for (int i = 0; i < Mix_AllocateChannels(-1); i++) {
			const char* name = "";
			Mix_Chunk* chunk = Mix_GetChunk(i);
			for (int j = 0; j < ArrayLength(chunks); j++) {
				if (chunk == chunks[j]) {
					name = chunk_names[j];
					break;
				}
			}
			char buf[100];
			SDL_snprintf(buf, sizeof(buf), "%d %.0f %d %s", i, channel_when_played[i], channel_priority[i], name);
			// SDL_Color col = Mix_Playing(i) ? SDL_Color{128, 255, 128, 255} : SDL_Color{255, 128, 128, 255};
			// SDL_Color col = Mix_Playing(i) ? SDL_Color{64, 255, 64, 255} : SDL_Color{255, 255, 255, 255};
			SDL_Color col = Mix_Playing(i) ? SDL_Color{255, 255, 255, 255} : SDL_Color{128, 128, 128, 255};
			DrawText(&fnt_mincho, buf, x, y, 0, 0, col);
			y += fnt_mincho.lineskip;
		}
	}

	/*
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);

	// SDL_RenderCopy(renderer, sprite_group.atlas_texture[0], nullptr, nullptr);
	SDL_RenderCopy(renderer, fnt_mincho.texture, nullptr, nullptr);
	//*/

	SDL_RenderPresent(renderer);
}
