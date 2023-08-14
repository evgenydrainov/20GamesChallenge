#include "Game.h"

#ifdef _WIN32
#include <SDL_ttf.h>
#else
#include <SDL2/SDL_ttf.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "mathh.h"

Game* game;

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
	if (type == 3) {
		e->radius = ASTEROID_RADIUS_3;
		e->type = 3;
		e->texture = game->tex_asteroid3;
	} else if (type == 2) {
		e->radius = ASTEROID_RADIUS_2;
		e->type = 2;
		e->texture = game->tex_asteroid2;
	} else if (type == 1) {
		e->radius = ASTEROID_RADIUS_1;
		e->type = 1;
		e->texture = game->tex_asteroid1;
	}
	return e;
}

static void wait(mco_coro* co, int t) {
	while (t--) {
		mco_yield(co);
	}
}

static Bullet* shoot(Enemy* e, float spd, float dir) {
	Bullet* b = game->CreateBullet();
	b->x = e->x;
	b->y = e->y;
	b->hsp = e->hsp + lengthdir_x(spd, dir);
	b->vsp = e->vsp + lengthdir_y(spd, dir);
	return b;
}

#define self(co) ((Enemy*)((co)->user_data))

static void enemy_ship(mco_coro* co) {
	while (true) {
		wait(co, 2 * 60);
		
		for (int i = 5; i--;) {
			shoot(self(co),
			  10.0f,
			  self(co)->angle);
			
			wait(co, 10);
		}
	}
}

static void spawn_enemies(mco_coro* co) {
	{
		int i = 200;
		while (i--) {
			float x = (float)(rand() % (int)MAP_W);
			float y = (float)(rand() % (int)MAP_H);
			float dist = point_distance(x, y, game->player.x, game->player.y);
			if (dist < 800.0f) {
				i++;
				continue;
			}

			float dir = (float)(rand() % 360);
			float spd = (float)(100 + rand() % 200) / 100.0f;
			make_asteroid(x, y, lengthdir_x(spd, dir), lengthdir_y(spd, dir), 1 + rand() % 3);
		}
	}

	//wait(co, 60 * 60);

	while (true) {
		{
			Enemy* e = game->CreateEnemy();
			
			e->x = game->player.x - lengthdir_x(900.0f, game->player.dir);
			e->y = game->player.y - lengthdir_y(900.0f, game->player.dir);
			e->hsp = lengthdir_x(0.1f, game->player.dir);
			e->vsp = lengthdir_y(0.1f, game->player.dir);
			e->angle = game->player.dir;

			e->type = 10;
			e->texture = game->tex_player_ship;
			mco_desc desc = mco_desc_init(enemy_ship, 0);
			mco_create(&e->co, &desc);
		}
		
		while (true) {
			int enemy_count = 0;
			for (int i = 0; i < game->enemy_count; i++) {
				if (game->enemies[i].type == 10) {
					enemy_count++;
				}
			}

			if (enemy_count == 0) {
				break;
			}

			wait(co, 1);
		}

		wait(co, 60);
	}
}

void Game::Init() {
	srand(::time(nullptr));

	SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);

	SDL_Init(SDL_INIT_VIDEO);
	IMG_Init(IMG_INIT_PNG);
	TTF_Init();
	Mix_Init(MIX_INIT_OGG);

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
	// SDL_SetHint(SDL_HINT_RENDER_DRIVER,        "opengl");
	// SDL_SetHint(SDL_HINT_RENDER_BATCHING,      "1");

	window = SDL_CreateWindow("04Asteroids",
							  SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
							  GAME_W, GAME_H,
							  SDL_WINDOW_RESIZABLE);

	renderer = SDL_CreateRenderer(window, -1,
								  SDL_RENDERER_ACCELERATED
								  | SDL_RENDERER_TARGETTEXTURE);

	SDL_RenderSetLogicalSize(renderer, GAME_W, GAME_H);

	tex_player_ship = IMG_LoadTexture(renderer, "assets/player_ship.png");
	tex_bg          = IMG_LoadTexture(renderer, "assets/bg.png"); // https://deep-fold.itch.io/space-background-generator
	tex_bg1         = IMG_LoadTexture(renderer, "assets/bg1.png");
	tex_asteroid1   = IMG_LoadTexture(renderer, "assets/asteroid1.png");
	tex_asteroid2   = IMG_LoadTexture(renderer, "assets/asteroid2.png");
	tex_asteroid3   = IMG_LoadTexture(renderer, "assets/asteroid3.png");
	tex_moon        = IMG_LoadTexture(renderer, "assets/moon.png");

	LoadFont(&fnt_mincho, "assets/mincho.ttf", 22);

	player.x = (float)MAP_W / 2.0f;
	player.y = (float)MAP_H / 2.0f;

	camera_x = player.x - (float)GAME_W / 2.0f;
	camera_y = player.y - (float)GAME_H / 2.0f;

	enemies   = (Enemy*)  malloc(sizeof(Enemy)  * MAX_ENEMIES);
	bullets   = (Bullet*) malloc(sizeof(Bullet) * MAX_BULLETS);
	p_bullets = (Bullet*) malloc(sizeof(Bullet) * MAX_PLR_BULLETS);

	mco_desc desc = mco_desc_init(spawn_enemies, 0);
	mco_create(&co, &desc);

	if (Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 2048) == 0) {
		printf("opened audio device\n");
	} else {
		printf("didn't open audio device\n");
	}

	// prev_time = GetTime() - (1.0 / (double)GAME_FPS);
}

void Game::Quit() {
	SDL_DestroyTexture(interface_map_texture);

	mco_destroy(co);

	free(p_bullets);
	free(bullets);
	free(enemies);

	DestroyFont(&fnt_mincho);

	SDL_DestroyTexture(tex_moon);
	SDL_DestroyTexture(tex_asteroid3);
	SDL_DestroyTexture(tex_asteroid2);
	SDL_DestroyTexture(tex_asteroid1);
	SDL_DestroyTexture(tex_bg1);
	SDL_DestroyTexture(tex_bg);
	SDL_DestroyTexture(tex_player_ship);

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

				//float max_spd = (e->catch_up_timer > 0.0f) ? 14.0f : 8.0f;
				float max_spd = 10.5f;

				// float spd = length(e->hsp, e->vsp);
				// float dir = point_direction(0.0f, 0.0f, e->hsp, e->vsp);
					
				// float dir_target = dir - angle_difference(dir, point_direction(e->x, e->y, rel_x, rel_y));
				// dir = approach(dir, dir_target, 1.5f * delta);
				// spd += 0.2f;
				// if (spd > max_spd) spd = max_spd;
					
				// e->hsp = lengthdir_x(spd, dir);
				// e->vsp = lengthdir_y(spd, dir);
				// e->angle = dir;

				float dir = point_direction(e->x, e->y, rel_x, rel_y);
				e->hsp += lengthdir_x(0.3f, dir) * delta;
				e->vsp += lengthdir_y(0.3f, dir) * delta;
				e->angle = point_direction(0.0f, 0.0f, e->hsp, e->vsp);
				if (length(e->hsp, e->vsp) > max_spd) {
					e->hsp = lengthdir_x(max_spd, e->angle);
					e->vsp = lengthdir_y(max_spd, e->angle);
				}
			}
		}
	}

	physics_update(delta);

	// :late update

	{
		Player* p = &player;
		if (input_press & INPUT_FIRE) {
			auto shoot = [p](float spd, float dir) {
				Bullet* pb = game->CreatePlrBullet();
				pb->x = p->x;
				pb->y = p->y;

				pb->x += lengthdir_x(20.0f, dir);
				pb->y += lengthdir_y(20.0f, dir);

				pb->hsp = p->hsp;
				pb->vsp = p->vsp;

				pb->hsp += lengthdir_x(spd, dir);
				pb->vsp += lengthdir_y(spd, dir);

				return pb;
			};

			if (p->power >= 50.0f) {
				shoot(15.0f, p->dir - 4.0f);
				shoot(15.0f, p->dir + 4.0f);
			} else {
				shoot(15.0f, p->dir);
			}
		}

		p->invincibility -= delta;
		if (p->invincibility < 0.0f) p->invincibility = 0.0f;

		{
			constexpr float f = 1.0f - 0.4f;
			float target_x = p->x - (float)GAME_W / camera_scale / 2.0f;
			float target_y = p->y - (float)GAME_H / camera_scale / 2.0f;
			camera_x = lerp(camera_x, target_x, 1.0f - SDL_powf(f, delta));
			camera_y = lerp(camera_y, target_y, 1.0f - SDL_powf(f, delta));
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

	auto decelerate = [p, delta](float dec) {
		float l = length(p->hsp, p->vsp);
		if (l > PLAYER_ACC) {
			p->hsp -= dec * p->hsp / l * delta;
			p->vsp -= dec * p->vsp / l * delta;
		} else {
			p->hsp = 0.0f;
			p->vsp = 0.0f;
		}
	};

	if (p->focus) {
		if (input & INPUT_RIGHT) p->x += PLAYER_FOCUS_SPD * delta;
		if (input & INPUT_UP)    p->y -= PLAYER_FOCUS_SPD * delta;
		if (input & INPUT_LEFT)  p->x -= PLAYER_FOCUS_SPD * delta;
		if (input & INPUT_DOWN)  p->y += PLAYER_FOCUS_SPD * delta;

		decelerate(PLAYER_ACC * 2.0f);

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
		} else {
			float dec = (input & INPUT_DOWN) ? PLAYER_ACC : (PLAYER_ACC / 16.0f);
			decelerate(dec);

			// if ((input & INPUT_DOWN) && length(p->hsp, p->vsp) < 0.1f) {
			// 	p->x -= lengthdir_x(2.0f, p->dir);
			// 	p->y -= lengthdir_y(2.0f, p->dir);
			// }
		}
	}

	// limit speed
	float player_spd = length(p->hsp, p->vsp);
	if (player_spd > PLAYER_MAX_SPD) {
		p->hsp = p->hsp / player_spd * PLAYER_MAX_SPD;
		p->vsp = p->vsp / player_spd * PLAYER_MAX_SPD;
	}
}

static bool enemy_get_damage(Enemy* e, float dmg, float split_dir) {
	e->health -= dmg;

	split_dir += (float)(-5 + rand() % 10);
				
	if (e->health <= 0.0f) {
		if (e->type == 3) {
			make_asteroid(e->x, e->y, e->hsp + lengthdir_x(1.0f, split_dir + 90.0f), e->vsp + lengthdir_y(1.0f, split_dir + 90.0f), 2, e->power / 2.0f);
			make_asteroid(e->x, e->y, e->hsp + lengthdir_x(1.0f, split_dir - 90.0f), e->vsp + lengthdir_y(1.0f, split_dir - 90.0f), 2, e->power / 2.0f);
		} else if (e->type == 2) {
			make_asteroid(e->x, e->y, e->hsp + lengthdir_x(1.0f, split_dir + 90.0f), e->vsp + lengthdir_y(1.0f, split_dir + 90.0f), 1, e->power / 2.0f);
			make_asteroid(e->x, e->y, e->hsp + lengthdir_x(1.0f, split_dir - 90.0f), e->vsp + lengthdir_y(1.0f, split_dir - 90.0f), 1, e->power / 2.0f);
		}

		return false;
	}

	return true;
}

static void player_get_dmg(Player* p, float dmg) {
	p->health -= dmg;
	p->invincibility = 60.0f;
}

void Game::physics_update(float delta) {
	{
		Player* p = &player;
		p->x += p->hsp * delta;
		p->y += p->vsp * delta;

		if (p->x < 0.0f) {p->x += MAP_W; camera_x += MAP_W;}
		if (p->y < 0.0f) {p->y += MAP_H; camera_y += MAP_H;}
		if (p->x >= MAP_W) {p->x -= MAP_W; camera_x -= MAP_W;}
		if (p->y >= MAP_H) {p->y -= MAP_H; camera_y -= MAP_H;}
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

	// :collision

	{
		Player* p = &player;

		if (p->invincibility == 0.0f) {
			for (int i = 0; i < bullet_count; i++) {
				Bullet* b = &bullets[i];
				if (circle_vs_circle(p->x, p->y, p->radius, b->x, b->y, b->radius)) {
					player_get_dmg(p, b->dmg);
					
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
					player_get_dmg(p, 1.0f);
					
					float split_dir = point_direction(p->x, p->y, e->x, e->y);
					if (!enemy_get_damage(e, 1.0f, split_dir)) {
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

		auto collide_with_bullets = [e](Bullet* bullets, int bullet_count, void (Game::*destroy)(int), bool give_power = false) {
			for (int bullet_idx = 0; bullet_idx < bullet_count;) {
				Bullet* b = &bullets[bullet_idx];

				if (circle_vs_circle(e->x, e->y, e->radius, b->x, b->y, b->radius)) {
					float split_dir = point_direction(b->x, b->y, e->x, e->y);

					float dmg = b->dmg;
					(game->*destroy)(bullet_idx);
					bullet_count--;

					if (!enemy_get_damage(e, dmg, split_dir)) {
						if (give_power) game->player.power += e->power;
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
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);

	// draw bg
	auto draw_bg = [](SDL_Texture* texture, float parallax) {
		float bg_w;
		float bg_h;
		{
			int w;
			int h;
			SDL_QueryTexture(texture, nullptr, nullptr, &w, &h);
			bg_w = (float)w;
			bg_h = (float)h;
		}
		float cam_x_para = game->camera_x / parallax;
		float cam_y_para = game->camera_y / parallax;
			
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
				SDL_RenderCopyF(game->renderer, texture, nullptr, &dest);

				x += bg_w;
			}

			y += bg_h;
		}
	};

	draw_bg(tex_bg, 5.0f);
	draw_bg(tex_bg1, 4.0f);

	{
		auto draw = [](float xoff, float yoff) {
			int w;
			int h;
			SDL_QueryTexture(game->tex_moon, nullptr, nullptr, &w, &h);
			SDL_FRect dest = {
				SDL_floorf(500.0f - (game->camera_x + xoff) / 5.0f),
				SDL_floorf(500.0f - (game->camera_y + yoff) / 5.0f),
				(float)w,
				(float)h
			};

			SDL_Rect idest = {(int)dest.x, (int)dest.y, (int)dest.w, (int)dest.h};
			SDL_Rect screen = {0, 0, GAME_W, GAME_H};
			if (!SDL_HasIntersection(&idest, &screen)) {
				return;
			}

			SDL_RenderCopyF(game->renderer, game->tex_moon, nullptr, &dest);
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
		DrawTextureCentered(e->texture, e->x, e->y, e->angle);
	}

	// draw player
	{
		SDL_Color col = {255, 255, 255, 255};
		if (player.invincibility > 0.0f) {
			if (((int)time / 4) % 2) col.a = 64;
			else col.a = 192;
		}
		DrawTextureCentered(tex_player_ship, player.x, player.y, player.dir, col);
	}

	for (int i = 0; i < bullet_count; i++) {
		Bullet* b = &bullets[i];
		DrawCircleCam(b->x, b->y, b->radius);
	}

	for (int i = 0; i < p_bullet_count; i++) {
		Bullet* pb = &p_bullets[i];
		DrawCircleCam(pb->x, pb->y, pb->radius);
	}

	// :ui
	if (!hide_interface || !interface_map_texture) {
		int map_w = 200;
		int map_h = 200;
		interface_update_timer -= delta;
		if (interface_update_timer <= 0.0f || !interface_map_texture) {
			interface_x = player.x;
			interface_y = player.y;

			if (!interface_map_texture) interface_map_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, map_w, map_h);
			SDL_SetRenderTarget(renderer, interface_map_texture);
			{
				SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
				SDL_RenderClear(renderer);

				for (int i = 0; i < enemy_count; i++) {
					SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
					if (1 <= enemies[i].type && enemies[i].type <= 3) {
						SDL_RenderDrawPoint(renderer,
											(int)(enemies[i].x / MAP_W * (float)map_w),
											(int)(enemies[i].y / MAP_H * (float)map_h));
					} else {
						SDL_Rect r = {
							(int)(enemies[i].x / MAP_W * (float)map_w),
							(int)(enemies[i].y / MAP_H * (float)map_h),
							2,
							2
						};
						SDL_RenderFillRect(renderer, &r);
					}
				}

				SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
				SDL_Rect r = {
					(int)(player.x / MAP_W * (float)map_w),
					(int)(player.y / MAP_H * (float)map_h),
					2,
					2
				};
				SDL_RenderFillRect(renderer, &r);
			}
			SDL_SetRenderTarget(renderer, nullptr);

			interface_update_timer = 5.0f;
		}

		int y;
		{
			char buf[100];
			SDL_snprintf(buf, sizeof(buf),
						 "X: %f\n"
						 "Y: %f\n"
						 "POWER: %.2f",
						 interface_x,
						 interface_y,
						 player.power);
			y = DrawText(&fnt_mincho, buf, 0, 0).y + fnt_mincho.height;
		}
		{
			// SDL_Rect dest = {0, y, map_w, map_h};
			// SDL_RenderCopy(renderer, interface_map_texture, nullptr, &dest);

			int w = 100;
			int h = 100;
			SDL_Rect src = {
				(int)(player.x / MAP_W * (float)map_w) - w / 2,
				(int)(player.y / MAP_H * (float)map_h) - h / 2,
				w,
				h
			};
			if (src.x < 0) src.x = 0;
			if (src.y < 0) src.y = 0;
			if (src.x > map_w - w - 1) src.x = map_w - w - 1;
			if (src.y > map_h - h - 1) src.y = map_h - h - 1;
			SDL_Rect dest = {10, y + 10, w, h};
			SDL_RenderCopy(renderer, interface_map_texture, &src, &dest);
		}

		// draw fps
		{
			char buf[10];
			SDL_snprintf(buf, sizeof(buf), "%.2f", fps);
			DrawText(&fnt_mincho, buf, GAME_W, GAME_H, HALIGN_RIGHT, VALIGN_BOTTOM);
		}
	}

	SDL_RenderPresent(renderer);
}

Enemy* Game::CreateEnemy() {
	if (enemy_count == MAX_ENEMIES) {
		SDL_Log("enemy limit hit");
		enemy_count--;
	}

	Enemy* result = &enemies[enemy_count];
	*result = {};
	enemy_count++;
	
	return result;
}

Bullet* Game::CreateBullet() {
	if (bullet_count == MAX_BULLETS) {
		SDL_Log("bullet limit hit");
		bullet_count--;
	}

	Bullet* result = &bullets[bullet_count];
	*result = {};
	bullet_count++;
	
	return result;
}

Bullet* Game::CreatePlrBullet() {
	if (p_bullet_count == MAX_PLR_BULLETS) {
		SDL_Log("plr bullet limit hit");
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
