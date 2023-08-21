#include "Game.h"

#ifdef _WIN32
#include <SDL_image.h>
#include <SDL_ttf.h>
#else
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#endif

#include <stdlib.h>
#include <stdio.h>

#include "mathh.h"
#include "misc.h"

#include "scripts.h"

#define INTERFACE_MAP_W 200
#define INTERFACE_MAP_H 200

Game* game;
static const char* chunk_names[ArrayLength(Game::chunks)];

static double GetTime() {
	return (double)SDL_GetPerformanceCounter() / (double)SDL_GetPerformanceFrequency();
}

static Enemy* make_asteroid(float x, float y, float hsp, float vsp, int type, float power = 1.0f) {
	Enemy* e = game->CreateEnemy();
	e->x = x;
	e->y = y;
	e->hsp = hsp;
	e->vsp = vsp;
	e->power = power;
	e->health = 1.0f;
	e->max_health = 1.0f;
	if (type == 3) {
		e->radius = ASTEROID_RADIUS_3;
		e->type = 3;
		e->sprite = &game->spr_asteroid3;
	} else if (type == 2) {
		e->radius = ASTEROID_RADIUS_2;
		e->type = 2;
		e->sprite = &game->spr_asteroid2;
	} else if (type == 1) {
		e->radius = ASTEROID_RADIUS_1;
		e->type = 1;
		e->sprite = &game->spr_asteroid1;
	}
	return e;
}

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

	// SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

	game_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_TARGET, GAME_W, GAME_H);
	SDL_SetTextureScaleMode(game_texture, SDL_ScaleModeLinear);

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

	for (int i = 0; i < Mix_AllocateChannels(-1); i++) {
		Mix_Volume(i, (int)(0.25f * (float)MIX_MAX_VOLUME));
	}

	player.x = (float)MAP_W / 2.0f;
	player.y = (float)MAP_H / 2.0f;

	camera_base_x = player.x - (float)GAME_W / 2.0f;
	camera_base_y = player.y - (float)GAME_H / 2.0f;

	enemies   = (Enemy*)  malloc(sizeof(Enemy)  * MAX_ENEMIES);
	bullets   = (Bullet*) malloc(sizeof(Bullet) * MAX_BULLETS);
	p_bullets = (Bullet*) malloc(sizeof(Bullet) * MAX_PLR_BULLETS);

	{
		int i = 200;
		while (i--) {
			float x = random.range(0.0f, MAP_W);
			float y = random.range(0.0f, MAP_H);
			float dist = point_distance(x, y, player.x, player.y);
			if (dist < 800.0f) {
				i++;
				continue;
			}

			float dir = random.range(0.0f, 360.0f);
			float spd = random.range(1.0f, 3.0f);
			make_asteroid(x, y, lengthdir_x(spd, dir), lengthdir_y(spd, dir), 1 + random.next() % 3);
		}
	}

	mco_desc desc = mco_desc_init(spawn_enemies, 0);
	mco_create(&co, &desc);

	// prev_time = GetTime() - (1.0 / (double)GAME_FPS);

	// printf("emscripten printf\n");
	// SDL_Log("emscripten SDL_Log\n");

	interface_map_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, INTERFACE_MAP_W, INTERFACE_MAP_H);
}

void Game::Quit() {
	SDL_DestroyTexture(interface_map_texture);

	mco_destroy(co);

	free(p_bullets);
	free(bullets);
	free(enemies);

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

static void DrawCircleCam(float x, float y, float radius, SDL_Color color = {255, 255, 255, 255}) {
	auto draw = [radius, color](float x, float y) {
		SDL_Rect contains = {(int)(x - radius - game->camera_x), (int)(y - radius - game->camera_y), (int)(radius * 2.0f), (int)(radius * 2.0f)};
		SDL_Rect screen = {0, 0, GAME_W, GAME_H};

		if (!SDL_HasIntersection(&contains, &screen)) {
			return;
		}
		
		constexpr int precision = 14;

		for (int i = 0; i < precision; i++) {
			float dir = (float)i / (float)precision * 360.0f;
			float dir1 = (float)(i + 1) / (float)precision * 360.0f;

			SDL_Vertex vertices[3] = {};
			vertices[0].position = {x, y};
			vertices[1].position = {x + lengthdir_x(radius, dir), y + lengthdir_y(radius, dir)};
			vertices[2].position = {x + lengthdir_x(radius, dir1), y + lengthdir_y(radius, dir1)};
			vertices[0].color = color;
			vertices[1].color = color;
			vertices[2].color = color;

			vertices[0].position.x -= game->camera_x;
			vertices[1].position.x -= game->camera_x;
			vertices[2].position.x -= game->camera_x;

			vertices[0].position.y -= game->camera_y;
			vertices[1].position.y -= game->camera_y;
			vertices[2].position.y -= game->camera_y;

			SDL_RenderGeometry(game->renderer, nullptr, vertices, ArrayLength(vertices), nullptr, 0);
		}
	};

	draw(x - MAP_W, y - MAP_H);
	draw(x,         y - MAP_H);
	draw(x + MAP_W, y - MAP_H);

	draw(x - MAP_W, y);
	draw(x,         y);
	draw(x + MAP_W, y);

	draw(x - MAP_W, y + MAP_H);
	draw(x,         y + MAP_H);
	draw(x + MAP_W, y + MAP_H);
}

template <typename Obj, typename F>
static Obj* find_closest(Obj* objects, int object_count,
						 float x, float y,
						 float* rel_x, float* rel_y, float* out_dist,
						 const F& filter) {
	Obj* result = nullptr;
	float dist_sq = INFINITY;

	for (int i = 0; i < object_count; i++) {
		if (!filter(&objects[i])) continue;

		auto check = [i, objects, x, y, &result, &dist_sq, rel_x, rel_y, out_dist](float xoff, float yoff) {
			float dx = x - (objects[i].x + xoff);
			float dy = y - (objects[i].y + yoff);

			float d = dx * dx + dy * dy;
			if (d < dist_sq) {
				result = &objects[i];
				*rel_x = objects[i].x + xoff;
				*rel_y = objects[i].y + yoff;
				dist_sq = d;
				*out_dist = SDL_sqrtf(dist_sq);
			}
		};

		check(-MAP_W, -MAP_H);
		check( 0.0f,  -MAP_H);
		check( MAP_W, -MAP_H);

		check(-MAP_W,  0.0f);
		check( 0.0f,   0.0f);
		check( MAP_W,  0.0f);

		check(-MAP_W,  MAP_H);
		check( 0.0f,   MAP_H);
		check( MAP_W,  MAP_H);
	}

	return result;
}

template <typename Obj>
static Obj* find_closest(Obj* objects, int object_count,
						 float x, float y,
						 float* rel_x, float* rel_y, float* out_dist) {
	return find_closest(objects, object_count,
						x, y,
						rel_x, rel_y, out_dist,
						[](Obj*) { return true; });
}

static void DrawTextureCentered(SDL_Texture* texture, float x, float y,
								float angle = 0.0f, SDL_Color col = {255, 255, 255, 255}) {
	int w;
	int h;
	SDL_QueryTexture(texture, nullptr, nullptr, &w, &h);

	auto draw = [x, y, w, h, texture, angle, col](float xoff, float yoff) {
		SDL_FRect dest = {
			x - (float)w / 2.0f - game->camera_x + xoff,
			y - (float)h / 2.0f - game->camera_y + yoff,
			(float)w,
			(float)h
		};

		SDL_Rect idest = {(int)dest.x, (int)dest.y, (int)dest.w, (int)dest.h};
		SDL_Rect screen = {0, 0, GAME_W, GAME_H};
		if (!SDL_HasIntersection(&idest, &screen)) {
			return;
		}

		SDL_SetTextureColorMod(texture, col.r, col.g, col.b);
		SDL_SetTextureAlphaMod(texture, col.a);
		SDL_RenderCopyExF(game->renderer, texture, nullptr, &dest, (double)(-angle), nullptr, SDL_FLIP_NONE);
	};

	draw(-MAP_W, -MAP_H);
	draw( 0.0f,  -MAP_H);
	draw( MAP_W, -MAP_H);

	draw(-MAP_W,  0.0f);
	draw( 0.0f,   0.0f);
	draw( MAP_W,  0.0f);

	draw(-MAP_W,  MAP_H);
	draw( 0.0f,   MAP_H);
	draw( MAP_W,  MAP_H);
}

static bool is_on_screen(float x, float y) {
	return (-100.0f <= x && x < (float)GAME_W + 100.0f)
		&& (-100.0f <= y && y < (float)GAME_H + 100.0f);
}

static void DrawSpriteCamWarped(Sprite* sprite, int frame_index,
					float x, float y,
					float angle = 0.0f, float xscale = 1.0f, float yscale = 1.0f,
					SDL_Color color = {255, 255, 255, 255}) {
	auto draw = [sprite, frame_index, x, y, angle, xscale, yscale, color](float xoff, float yoff) {
		float xx = x - game->camera_x;
		float yy = y - game->camera_y;

		xx += xoff;
		yy += yoff;

		if (!is_on_screen(xx, yy)) return;

		DrawSprite(sprite, frame_index, xx, yy, angle, xscale, yscale, color);
	};

	draw(-MAP_W, -MAP_H);
	draw( 0.0f,  -MAP_H);
	draw( MAP_W, -MAP_H);

	draw(-MAP_W,  0.0f);
	draw( 0.0f,   0.0f);
	draw( MAP_W,  0.0f);

	draw(-MAP_W,  MAP_H);
	draw( 0.0f,   MAP_H);
	draw( MAP_W,  MAP_H);
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

	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
			case SDL_QUIT: {
				quit = true;
				break;
			}

			case SDL_KEYDOWN: {
				switch (event.key.keysym.scancode) {
					case SDL_SCANCODE_TAB: {
						hide_interface ^= true;
						break;
					}
					case SDL_SCANCODE_H: {
						show_hitboxes ^= true;
						break;
					}
				}
				break;
			}
		}
	}

	float delta = 1.0f;

	fps_sum += current_fps;
	fps_timer += delta;
	if (fps_timer >= 60.0f) {
		fps = fps_sum / fps_timer;
		fps_timer = 0.0f;
		fps_sum = 0.0f;
	}

	// input
	{
		const Uint8* key = SDL_GetKeyboardState(nullptr);

		unsigned int prev = input;
		input = 0;

		input |= INPUT_RIGHT * key[SDL_SCANCODE_RIGHT];
		input |= INPUT_UP    * key[SDL_SCANCODE_UP];
		input |= INPUT_LEFT  * key[SDL_SCANCODE_LEFT];
		input |= INPUT_DOWN  * key[SDL_SCANCODE_DOWN];
		input |= INPUT_FIRE  * key[SDL_SCANCODE_Z];
		input |= INPUT_FOCUS * key[SDL_SCANCODE_LSHIFT];
		input |= INPUT_BOOST * key[SDL_SCANCODE_LCTRL];

		input_press   = ~prev &  input;
		input_release =  prev & ~input;
	}

	update(delta);

	draw(delta);

	time += delta;

#ifndef __EMSCRIPTEN__
	t = GetTime();
	double time_left = frame_end_time - t;
	if (time_left > 0.0) {
		SDL_Delay((Uint32)(time_left * 0.95 * 1000.0));
		while (GetTime() < frame_end_time) {}
	}
#endif
}

template <typename Obj>
static void decelerate(Obj* o, float dec, float delta) {
	float l = length(o->hsp, o->vsp);
	if (l > dec) {
		o->hsp -= dec * o->hsp / l * delta;
		o->vsp -= dec * o->vsp / l * delta;
	} else {
		o->hsp = 0.0f;
		o->vsp = 0.0f;
	}
};

void Game::update(float delta) {
	update_player(delta);

	for (int i = 0; i < enemy_count; i++) {
		Enemy* e = &enemies[i];
		if (e->type == 10) {
			e->catch_up_timer -= delta;
			if (e->catch_up_timer < 0.0f) e->catch_up_timer = 0.0f;

			float rel_x;
			float rel_y;
			float dist;
			if (Player* p = find_closest(&player, 1, e->x, e->y, &rel_x, &rel_y, &dist)) {
				if (dist > 800.0f && e->catch_up_timer == 0.0f) {
					e->catch_up_timer = 5.0f * 60.0f;
				}

				float dir = point_direction(e->x, e->y, rel_x, rel_y);
				if (dist < 200.0f && length(p->hsp, p->vsp) < 5.0f) {
					decelerate(e, 0.1f, delta);

					// e->angle -= clamp(angle_difference(e->angle, dir), ) * delta;
					e->angle = approach(e->angle, e->angle - angle_difference(e->angle, dir), 5.0f * delta);
				} else {
					if (SDL_fabsf(angle_difference(e->angle, dir)) > 20.0f) {
						e->hsp += lengthdir_x(e->acc, dir) * delta;
						e->vsp += lengthdir_y(e->acc, dir) * delta;
						e->angle = point_direction(0.0f, 0.0f, e->hsp, e->vsp);
					} else {
						e->hsp += lengthdir_x(e->acc, e->angle) * delta;
						e->vsp += lengthdir_y(e->acc, e->angle) * delta;
					}
					
					if (length(e->hsp, e->vsp) > e->max_spd) {
						e->hsp = lengthdir_x(e->max_spd, e->angle);
						e->vsp = lengthdir_y(e->max_spd, e->angle);
					}
				}
			}
		}

		AnimateSprite(e->sprite, &e->frame_index, delta);
	}

	physics_update(delta);

	// :late update

	{
		Player* p = &player;
		p->fire_timer += delta;
		while (p->fire_timer >= 10.0f) {
			if (input & INPUT_FIRE) {
				if (p->fire_queue == 0) p->fire_queue = 4;
			}

			if (p->fire_queue > 0) {
				auto shoot = [this, p](float spd, float dir, float hoff = 0.0f) {
					Bullet* pb = CreatePlrBullet();
					pb->x = p->x;
					pb->y = p->y;

					// pb->x += lengthdir_x(20.0f, dir);
					// pb->y += lengthdir_y(20.0f, dir);

					pb->x += lengthdir_x(hoff, dir - 90.0f);
					pb->y += lengthdir_y(hoff, dir - 90.0f);

					pb->hsp = p->hsp;
					pb->vsp = p->vsp;

					pb->hsp += lengthdir_x(spd, dir);
					pb->vsp += lengthdir_y(spd, dir);

					stop_sound(snd_player_shoot);
					play_sound(snd_shoot, p->x, p->y); // play_sound(snd_player_shoot, p->x, p->y);

					return pb;
				};

				switch (p->power_level) {
					case 1: {
						shoot(20.0f, p->dir);
						break;
					}
					case 2: {
						shoot(20.0f, p->dir - 4.0f);
						shoot(20.0f, p->dir + 4.0f);
						break;
					}
					case 3: {
						shoot(20.0f, p->dir,  10.0f);
						shoot(20.0f, p->dir, -10.0f);

						shoot(20.0f, p->dir - 30.0f,  10.0f);
						shoot(20.0f, p->dir + 30.0f, -10.0f);
						break;
					}
				}

				p->fire_queue--;
			}

			p->fire_timer -= 10.0f;
		}
		
		p->invincibility -= delta;
		if (p->invincibility < 0.0f) p->invincibility = 0.0f;

		{
			constexpr float f = 1.0f - 0.02f;
			float target_x = p->x - (float)GAME_W / camera_scale / 2.0f;
			float target_y = p->y - (float)GAME_H / camera_scale / 2.0f;

			target_x += lengthdir_x(100.0f, p->dir);
			target_y += lengthdir_y(100.0f, p->dir);

			camera_base_x += p->hsp;
			camera_base_y += p->vsp;

			camera_base_x = lerp(camera_base_x, target_x, 1.0f - SDL_powf(f, delta));
			camera_base_y = lerp(camera_base_y, target_y, 1.0f - SDL_powf(f, delta));

			screenshake_timer -= delta;
			if (screenshake_timer < 0.0f) screenshake_timer = 0.0f;

			camera_x = camera_base_x;
			camera_y = camera_base_y;

			if (screenshake_timer > 0.0f) {
				// float f = screenshake_timer / screenshake_time;
				// float range = screenshake_intensity * f;
				float range = screenshake_intensity;
				camera_x += random.range(-range, range);
				camera_y += random.range(-range, range);
			}
		}
	}

	for (int i = 0; i < bullet_count; i++) {
		Bullet* b = &bullets[i];
		b->lifetime += delta;
		if (b->lifetime >= b->lifespan) {
			DestroyBullet(i);
			i--;
		}
	}

	for (int i = 0; i < p_bullet_count; i++) {
		Bullet* pb = &p_bullets[i];
		pb->lifetime += delta;
		if (pb->lifetime >= pb->lifespan) {
			DestroyPlrBullet(i);
			i--;
		}
	}

	if (mco_status(co) != MCO_DEAD) {
		mco_resume(co);
	}

	for (int i = 0; i < enemy_count; i++) {
		Enemy* e = &enemies[i];
		if (e->co) {
			if (mco_status(e->co) != MCO_DEAD) {
				e->co->user_data = e;
				mco_resume(e->co);
			}
		}
	}
}

void Game::update_player(float delta) {
	Player* p = &player;

	p->focus = (input & INPUT_FOCUS) != 0;

	if (p->focus) {
		if (input & INPUT_RIGHT) {p->x += PLAYER_FOCUS_SPD * delta; camera_base_x += PLAYER_FOCUS_SPD * delta;}
		if (input & INPUT_UP)    {p->y -= PLAYER_FOCUS_SPD * delta; camera_base_y -= PLAYER_FOCUS_SPD * delta;}
		if (input & INPUT_LEFT)  {p->x -= PLAYER_FOCUS_SPD * delta; camera_base_x -= PLAYER_FOCUS_SPD * delta;}
		if (input & INPUT_DOWN)  {p->y += PLAYER_FOCUS_SPD * delta; camera_base_y += PLAYER_FOCUS_SPD * delta;}

		decelerate(p, PLAYER_ACC * 2.0f, delta);

		float rel_x;
		float rel_y;
		float dist;
		if (Enemy* e = find_closest(enemies, enemy_count,
									p->x, p->y,
									&rel_x, &rel_y, &dist,
									[](Enemy* e) { return !(1 <= e->type && e->type <= 3); })) {
			if (dist < 800.0f) {
				constexpr float f = 1.0f - 0.05f;
				float dir = point_direction(p->x, p->y, rel_x, rel_y);
				float target = p->dir - angle_difference(p->dir, dir);
				p->dir = lerp(p->dir, target, 1.0f - SDL_powf(f, delta));
			}
		}
	} else {
		{
			float turn_spd = PLAYER_TURN_SPD;
			if ((input & INPUT_UP) || (input & INPUT_DOWN)) {
				turn_spd /= 2.0f;
			}
			if (input & INPUT_RIGHT) p->dir -= turn_spd * delta;
			if (input & INPUT_LEFT)  p->dir += turn_spd * delta;
		}
				
		if (input & INPUT_UP) {
			// accelerate
			float rad = p->dir / 180.0f * (float)M_PI;
			p->hsp += PLAYER_ACC *  SDL_cosf(rad) * delta;
			p->vsp += PLAYER_ACC * -SDL_sinf(rad) * delta;

			if ((int)time % 5 == 0) {
				stop_sound(snd_ship_engine);
				play_sound(snd_ship_engine, p->x, p->y);
			}
		} else {
			float dec = (input & INPUT_DOWN) ? PLAYER_ACC : (PLAYER_ACC / 16.0f);
			decelerate(p, dec, delta);

			// if ((input & INPUT_DOWN) && length(p->hsp, p->vsp) < 0.1f) {
			// 	p->x -= lengthdir_x(2.0f, p->dir);
			// 	p->y -= lengthdir_y(2.0f, p->dir);
			// }
		}
	}

	// limit speed
	float player_spd = length(p->hsp, p->vsp);
	float max_spd = (input & INPUT_BOOST) ? 14.0f : PLAYER_MAX_SPD;
	if (player_spd > max_spd) {
		p->hsp = p->hsp / player_spd * max_spd;
		p->vsp = p->vsp / player_spd * max_spd;
	}

	p->health += (1.0f / 60.0f) * delta;
	if (p->health > p->max_health) p->health = p->max_health;
}

static bool enemy_get_hit(Enemy* e, float dmg, float split_dir) {
	e->health -= dmg;

	split_dir += game->random.range(-5.0f, 5.0f);

	play_sound(game->snd_hurt, e->x, e->y);

	if (e->health <= 0.0f) {
		switch (e->type) {
			case 3: {
				make_asteroid(e->x, e->y, e->hsp + lengthdir_x(1.0f, split_dir + 90.0f), e->vsp + lengthdir_y(1.0f, split_dir + 90.0f), 2, e->power / 2.0f);
				make_asteroid(e->x, e->y, e->hsp + lengthdir_x(1.0f, split_dir - 90.0f), e->vsp + lengthdir_y(1.0f, split_dir - 90.0f), 2, e->power / 2.0f);
				break;
			}
			case 2: {
				make_asteroid(e->x, e->y, e->hsp + lengthdir_x(1.0f, split_dir + 90.0f), e->vsp + lengthdir_y(1.0f, split_dir + 90.0f), 1, e->power / 2.0f);
				make_asteroid(e->x, e->y, e->hsp + lengthdir_x(1.0f, split_dir - 90.0f), e->vsp + lengthdir_y(1.0f, split_dir - 90.0f), 1, e->power / 2.0f);
				break;
			}
			case 10: {
				game->screenshake_intensity = 5.0f;
				game->screenshake_time = 10.0f;
				game->screenshake_timer = 10.0f;
				SDL_Delay(40);
				break;
			}
			case 20: {
				game->screenshake_intensity = 10.0f;
				game->screenshake_time = 30.0f;
				game->screenshake_timer = 30.0f;
				SDL_Delay(50);
				break;
			}
		}

		if (e->type == 20) play_sound(game->snd_boss_explode, e->x, e->y);
		else play_sound(game->snd_explode, e->x, e->y);

		return false;
	}

	return true;
}

static void player_get_hit(Player* p, float dmg) {
	p->health -= dmg;
	p->invincibility = 60.0f;

	game->screenshake_intensity = 5.0f;
	game->screenshake_time = 10.0f;
	game->screenshake_timer = 10.0f;
	SDL_Delay(40);

	stop_sound(game->snd_hurt);
	play_sound(game->snd_hurt, p->x, p->y);
}

void Game::physics_update(float delta) {
	{
		Player* p = &player;
		p->x += p->hsp * delta;
		p->y += p->vsp * delta;

		if (p->x < 0.0f) {p->x += MAP_W; camera_base_x += MAP_W;}
		if (p->y < 0.0f) {p->y += MAP_H; camera_base_y += MAP_H;}
		if (p->x >= MAP_W) {p->x -= MAP_W; camera_base_x -= MAP_W;}
		if (p->y >= MAP_H) {p->y -= MAP_H; camera_base_y -= MAP_H;}
	}

	for (int i = 0; i < enemy_count; i++) {
		Enemy* e = &enemies[i];
			
		e->x += e->hsp * delta;
		e->y += e->vsp * delta;

		if (e->x < 0.0f) e->x += MAP_W;
		if (e->y < 0.0f) e->y += MAP_H;
		if (e->x >= MAP_W) e->x -= MAP_W;
		if (e->y >= MAP_H) e->y -= MAP_H;
	}

	for (int i = 0; i < bullet_count; i++) {
		Bullet* b = &bullets[i];

		b->x += b->hsp * delta;
		b->y += b->vsp * delta;

		if (b->x < 0.0f) b->x += MAP_W;
		if (b->y < 0.0f) b->y += MAP_H;
		if (b->x >= MAP_W) b->x -= MAP_W;
		if (b->y >= MAP_H) b->y -= MAP_H;
	}

	for (int i = 0; i < p_bullet_count; i++) {
		Bullet* pb = &p_bullets[i];

		pb->x += pb->hsp * delta;
		pb->y += pb->vsp * delta;

		if (pb->x < 0.0f) pb->x += MAP_W;
		if (pb->y < 0.0f) pb->y += MAP_H;
		if (pb->x >= MAP_W) pb->x -= MAP_W;
		if (pb->y >= MAP_H) pb->y -= MAP_H;
	}

	// :collision :coll

	{
		Player* p = &player;

		if (p->invincibility == 0.0f) {
			for (int i = 0; i < bullet_count; i++) {
				Bullet* b = &bullets[i];
				if (circle_vs_circle(p->x, p->y, p->radius, b->x, b->y, b->radius)) {
					player_get_hit(p, b->dmg);
					
					DestroyBullet(i);
					i--;

					break;
				}
			}
		}

		if (p->invincibility == 0.0f) {
			for (int i = 0, c = enemy_count; i < c; i++) {
				Enemy* e = &enemies[i];

				// if (!(1 <= e->type && e->type <= 3)) continue;

				if (circle_vs_circle(p->x, p->y, p->radius, e->x, e->y, e->radius)) {
					player_get_hit(p, 10.0f);
					
					float split_dir = point_direction(p->x, p->y, e->x, e->y);
					if (!enemy_get_hit(e, 10.0f, split_dir)) {
						DestroyEnemy(i);
						c--;
						i--;
					}

					break;
				}
			}
		}
	}

	for (int enemy_idx = 0, c = enemy_count; enemy_idx < c;) {
		Enemy* e = &enemies[enemy_idx];

		auto collide_with_bullets = [this, e](Bullet* bullets, int bullet_count, void (Game::*destroy)(int), bool give_power = false) {
			for (int bullet_idx = 0; bullet_idx < bullet_count;) {
				Bullet* b = &bullets[bullet_idx];

				if (circle_vs_circle(e->x, e->y, e->radius, b->x, b->y, b->radius)) {
					float split_dir = point_direction(b->x, b->y, e->x, e->y);

					float dmg = b->dmg;
					(this->*destroy)(bullet_idx);
					bullet_count--;

					if (!enemy_get_hit(e, dmg, split_dir)) {
						if (give_power) {
							player.power += e->power;
							if (player.power >= 200.0f) {
								if (player.power_level < 3) {player.power_level = 3; play_sound(snd_powerup, player.x, player.y);}
							} else if (player.power >= 50.0f) {
								if (player.power_level < 2) {player.power_level = 2; play_sound(snd_powerup, player.x, player.y);}
							}
						}
						return false;
					}
					
					continue;
				}

				bullet_idx++;
			}

			return true;
		};

		if (!collide_with_bullets(p_bullets, p_bullet_count, &Game::DestroyPlrBullet, true)) {
			DestroyEnemy(enemy_idx);
			c--;
			continue;
		}

		// if (1 <= e->type && e->type <= 3) {
		// 	if (!collide_with_bullets(bullets, bullet_count, &Game::DestroyBullet)) {
		// 		DestroyEnemy(enemy_idx);
		// 		c--;
		// 		continue;
		// 	}
		// }
			
		enemy_idx++;
	}
}

void Game::draw(float delta) {
	SDL_RenderSetLogicalSize(renderer, GAME_W, GAME_H);

	SDL_SetRenderTarget(renderer, game_texture);

	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);

	// draw bg
	auto draw_bg = [this](SDL_Texture* texture, float parallax) {
		float bg_w;
		float bg_h;
		{
			int w;
			int h;
			SDL_QueryTexture(texture, nullptr, nullptr, &w, &h);
			bg_w = (float) w;
			bg_h = (float) h;
		}
		float cam_x_para = camera_x / parallax;
		float cam_y_para = camera_y / parallax;
			
		float y = SDL_floorf(cam_y_para / bg_h) * bg_h;
		while (y < cam_y_para + (float)GAME_H) {
			float x = SDL_floorf(cam_x_para / bg_w) * bg_w;
			while (x < cam_x_para + (float)GAME_W) {
				SDL_FRect dest = {
					SDL_floorf(x - cam_x_para),
					SDL_floorf(y - cam_y_para),
					bg_w,
					bg_h
				};
				SDL_RenderCopyF(renderer, texture, nullptr, &dest);

				x += bg_w;
			}

			y += bg_h;
		}
	};

	draw_bg(tex_bg, 5.0f);
	draw_bg(tex_bg1, 4.0f);

	// draw moon
	{
		auto draw = [this](float xoff, float yoff) {
			int w;
			int h;
			SDL_QueryTexture(tex_moon, nullptr, nullptr, &w, &h);
			SDL_FRect dest = {
				SDL_floorf(500.0f - (camera_x + xoff) / 5.0f),
				SDL_floorf(500.0f - (camera_y + yoff) / 5.0f),
				(float) w,
				(float) h
			};

			SDL_Rect idest = {(int)dest.x, (int)dest.y, (int)dest.w, (int)dest.h};
			SDL_Rect screen = {0, 0, GAME_W, GAME_H};
			if (!SDL_HasIntersection(&idest, &screen)) {
				return;
			}

			SDL_RenderCopyF(renderer, tex_moon, nullptr, &dest);
		};

		draw(-MAP_W, -MAP_H);
		draw( 0.0f,  -MAP_H);
		draw( MAP_W, -MAP_H);

		draw(-MAP_W,  0.0f);
		draw( 0.0f,   0.0f);
		draw( MAP_W,  0.0f);

		draw(-MAP_W,  MAP_H);
		draw( 0.0f,   MAP_H);
		draw( MAP_W,  MAP_H);
	}

	// draw enemies
	for (int i = 0; i < enemy_count; i++) {
		Enemy* e = &enemies[i];
		if (show_hitboxes) DrawCircleCam(e->x, e->y, e->radius, {255, 0, 0, 255});
		DrawSpriteCamWarped(e->sprite, e->frame_index, e->x, e->y, e->angle);
	}

	// draw player
	{
		SDL_Color col = {255, 255, 255, 255};
		if (player.invincibility > 0.0f) {
			if (((int)time / 4) % 2) col.a = 64;
			else col.a = 192;
		}
		if (show_hitboxes) DrawCircleCam(player.x, player.y, player.radius, {128, 255, 128, 255});
		DrawSpriteCamWarped(&spr_player_ship, 0.0f, player.x, player.y, player.dir, 1.0f, 1.0f, col);
	}

	for (int i = 0; i < bullet_count; i++) {
		Bullet* b = &bullets[i];
		DrawCircleCam(b->x, b->y, b->radius);
	}

	for (int i = 0; i < p_bullet_count; i++) {
		Bullet* pb = &p_bullets[i];
		DrawCircleCam(pb->x, pb->y, pb->radius);
	}

	if (!hide_interface) {
		draw_ui(delta);
	} else {
		interface_update_timer = 0.0f;
	}

	SDL_SetRenderTarget(renderer, nullptr);

	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);

	SDL_RenderCopy(renderer, game_texture, nullptr, nullptr);

	{
		int window_w;
		int window_h;
		SDL_GetWindowSize(window, &window_w, &window_h);
		SDL_RenderSetLogicalSize(renderer, window_w, window_h);
	}
	
	int x = 0;
	int y = 0;
	if (show_audio_channels) {
		// draw audio channels

		for (int i = 0; i < Mix_AllocateChannels(-1); i++) {
			const char* name = "";
			if (Mix_Playing(i)) {
				Mix_Chunk* chunk = Mix_GetChunk(i);
				for (int j = 0; j < ArrayLength(chunks); j++) {
					if (chunk == chunks[j]) {
						name = chunk_names[j];
						break;
					}
				}
			}
			char buf[100];
			SDL_snprintf(buf, sizeof(buf), "%d %s", i, name);
			DrawText(&fnt_mincho, buf, x, y);
			y += 22;
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

void Game::draw_ui(float delta) {
	// :ui

	interface_update_timer -= delta;
	if (interface_update_timer <= 0.0f) {
		interface_x = player.x;
		interface_y = player.y;

		SDL_Texture* old_target = SDL_GetRenderTarget(renderer);
		SDL_SetRenderTarget(renderer, interface_map_texture);
		{
			SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
			SDL_RenderClear(renderer);

			for (int i = 0; i < enemy_count; i++) {
				SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
				if (1 <= enemies[i].type && enemies[i].type <= 3) {
					SDL_RenderDrawPoint(renderer,
										(int) (enemies[i].x / MAP_W * (float)INTERFACE_MAP_W),
										(int) (enemies[i].y / MAP_H * (float)INTERFACE_MAP_H));
				} else {
					SDL_Rect r = {
						(int) (enemies[i].x / MAP_W * (float)INTERFACE_MAP_W),
						(int) (enemies[i].y / MAP_H * (float)INTERFACE_MAP_H),
						2,
						2
					};
					SDL_RenderFillRect(renderer, &r);
				}
			}

			SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
			SDL_Rect r = {
				(int) (player.x / MAP_W * (float)INTERFACE_MAP_W),
				(int) (player.y / MAP_H * (float)INTERFACE_MAP_H),
				2,
				2
			};
			SDL_RenderFillRect(renderer, &r);
		}
		SDL_SetRenderTarget(renderer, old_target);

		interface_update_timer = 5.0f;
	}

	int x = 0;
	int y = 0;
	{
		char buf[100];
		SDL_snprintf(buf, sizeof(buf),
					 "X: %f\n"
					 "Y: %f\n"
					 "POWER: %.2f",
					 interface_x,
					 interface_y,
					 player.power);
		y += DrawText(&fnt_mincho, buf, x, y).y + fnt_mincho.height;
	}
	{
		int w = 180;
		int h = 22;
		SDL_Rect back = {x + 5, y + 5, w, h};
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderFillRect(renderer, &back);
		int filled_w = (int) (player.health / player.max_health * (float)w);
		SDL_Rect filled = {back.x, back.y, filled_w, back.h};
		SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
		SDL_RenderFillRect(renderer, &filled);

		char buf[10];
		SDL_snprintf(buf, sizeof(buf), "%.0f/%.0f", player.health, player.max_health);
		DrawText(&fnt_mincho, buf, back.x + w / 2, back.y + h / 2, HALIGN_CENTER, VALIGN_MIDDLE);

		y += h + 5 * 2;
	}
	{
		// SDL_Rect dest = {0, y, INTERFACE_MAP_W, INTERFACE_MAP_H};
		// SDL_RenderCopy(renderer, interface_map_texture, nullptr, &dest);

		int w = 100;
		int h = 100;
		SDL_Rect src = {
			(int) (player.x / MAP_W * (float)INTERFACE_MAP_W) - w / 2,
			(int) (player.y / MAP_H * (float)INTERFACE_MAP_H) - h / 2,
			w,
			h
		};
		if (src.x < 0) src.x = 0;
		if (src.y < 0) src.y = 0;
		if (src.x > INTERFACE_MAP_W - w - 1) src.x = INTERFACE_MAP_W - w - 1;
		if (src.y > INTERFACE_MAP_H - h - 1) src.y = INTERFACE_MAP_H - h - 1;
		SDL_Rect dest = {x + 5, y + 5, w, h};
		SDL_RenderCopy(renderer, interface_map_texture, &src, &dest);

		y += h + 5 * 2;
	}

	// draw boss healthbar
	for (int i = 0; i < enemy_count; i++) {
		if (!(enemies[i].type == 20)) continue;

		int w = 450;
		int h = 18;
		SDL_Rect back = {(GAME_W - w) / 2, 20, w, h};
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderFillRect(renderer, &back);
		int filled_w = (int) (enemies[i].health / enemies[i].max_health * (float)w);
		SDL_Rect filled = {back.x, back.y, filled_w, back.h};
		SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
		SDL_RenderFillRect(renderer, &filled);

		char buf[10];
		SDL_snprintf(buf, sizeof(buf), "%.0f/%.0f", enemies[i].health, enemies[i].max_health);
		DrawText(&fnt_mincho, buf, back.x + w / 2, back.y + h / 2, HALIGN_CENTER, VALIGN_MIDDLE);

		break;
	}

	// draw fps
	{
		char buf[10];
		SDL_snprintf(buf, sizeof(buf), "%.2f", fps);
		DrawText(&fnt_mincho, buf, GAME_W, GAME_H, HALIGN_RIGHT, VALIGN_BOTTOM);
	}
}

Enemy* Game::CreateEnemy() {
	if (enemy_count == MAX_ENEMIES) {
		printf("enemy limit hit\n");
		enemy_count--;
	}

	Enemy* result = &enemies[enemy_count];
	*result = {};
	enemy_count++;
	
	return result;
}

Bullet* Game::CreateBullet() {
	if (bullet_count == MAX_BULLETS) {
		printf("bullet limit hit\n");
		bullet_count--;
	}

	Bullet* result = &bullets[bullet_count];
	*result = {};
	bullet_count++;
	
	return result;
}

Bullet* Game::CreatePlrBullet() {
	if (p_bullet_count == MAX_PLR_BULLETS) {
		printf("plr bullet limit hit\n");
		p_bullet_count--;
	}

	Bullet* result = &p_bullets[p_bullet_count];
	*result = {};
	p_bullet_count++;
	
	return result;
}

void Game::DestroyEnemy(int enemy_idx) {
	if (enemies[enemy_idx].co) {
		mco_destroy(enemies[enemy_idx].co);
	}

	for (int i = enemy_idx + 1; i < enemy_count; i++) {
		enemies[i - 1] = enemies[i];
	}
	enemy_count--;
}

void Game::DestroyBullet(int bullet_idx) {
	for (int i = bullet_idx + 1; i < bullet_count; i++) {
		bullets[i - 1] = bullets[i];
	}
	bullet_count--;
}

void Game::DestroyPlrBullet(int p_bullet_idx) {
	for (int i = p_bullet_idx + 1; i < p_bullet_count; i++) {
		p_bullets[i - 1] = p_bullets[i];
	}
	p_bullet_count--;
}
