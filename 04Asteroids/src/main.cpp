//
// https://20_games_challenge.gitlab.io/games/asteroids/
//

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include <time.h>

#include "mathh.h"

#define GAME_W 1280
#define GAME_H 720
#define GAME_FPS 60
#define PLAYER_ACC 0.3f
#define PLAYER_MAX_SPD 10.0f
#define PLAYER_FOCUS_SPD 3.0f
#define PLAYER_TURN_SPD 4.0f
#define MAX_ENEMIES 100
#define MAX_BULLETS 1000
#define MAX_PLR_BULLETS 1000
#define ArrayLength(a) (sizeof(a) / sizeof(*a))
#define MAP_W 10'000.0f
#define MAP_H 10'000.0f
#define ASTEROID_RADIUS_3 50.0f
#define ASTEROID_RADIUS_2 25.0f
#define ASTEROID_RADIUS_1 12.0f

struct Player {
	float x;
	float y;
	float hsp;
	float vsp;
	float dir;
	bool focus;
};

struct Enemy {
	float x;
	float y;
	float hsp;
	float vsp;
	float radius = 10.0f;
	int type;
	float hp = 1.0f;
	SDL_Texture* texture;
};

struct Bullet {
	float x;
	float y;
	float hsp;
	float vsp;
	float radius = 5.0f;
	float dmg = 1.0f;
	float lifespan = 2.5f * 60.0f;
	float lifetime;
};

struct Game {
	Player player;
	float camera_x;
	float camera_y;
	Enemy* enemies;
	int enemy_count;
	Bullet* bullets;
	int bullet_count;
	Bullet* p_bullets; // player bullets
	int p_bullet_count;
	unsigned int input;
	unsigned int input_press;
	unsigned int input_release;

	SDL_Texture* tex_player_ship;
	SDL_Texture* tex_bg;
	SDL_Texture* tex_bg1;
	SDL_Texture* tex_asteroid1;
	SDL_Texture* tex_asteroid2;
	SDL_Texture* tex_asteroid3;
	SDL_Texture* tex_moon;

	TTF_Font* fnt_mincho;

	SDL_Window* window;
	SDL_Renderer* renderer;
	double prev_time;
	bool quit;
	double fps_sum;
	double fps_timer;
	SDL_Texture* fps_texture;
	float interface_update_t;
	SDL_Texture* interface_texture;
	SDL_Texture* interface_map_texture;
	bool hide_interface;

	void Init();
	void Quit();
	void Run();
	void Frame();

	Enemy* CreateEnemy();
	Bullet* CreateBullet();
	Bullet* CreatePlrBullet();

	void DestroyEnemy(int enemy_idx);
	void DestroyBullet(int bullet_idx);
	void DestroyPlrBullet(int p_bullet_idx);
};

enum {
	INPUT_RIGHT = 1,
	INPUT_UP    = 1 << 1,
	INPUT_LEFT  = 1 << 2,
	INPUT_DOWN  = 1 << 3,
	INPUT_FIRE  = 1 << 4,
	INPUT_FOCUS = 1 << 5
};

Game* game;

static double GetTime() {
	return (double)SDL_GetPerformanceCounter() / (double)SDL_GetPerformanceFrequency();
}

void Game::Init() {
	srand(time(nullptr));

	SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);

	SDL_Init(SDL_INIT_VIDEO);
	IMG_Init(IMG_INIT_PNG);
	TTF_Init();
	Mix_Init(MIX_INIT_OGG);

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

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

	fnt_mincho = TTF_OpenFont("assets/mincho.ttf", 22);

	player.x = (float)MAP_W / 2.0f;
	player.y = (float)MAP_H / 2.0f;

	camera_x = player.x - (float)GAME_W / 2.0f;
	camera_y = player.y - (float)GAME_H / 2.0f;

	enemies   = (Enemy*)  malloc(sizeof(Enemy)  * MAX_ENEMIES);
	bullets   = (Bullet*) malloc(sizeof(Bullet) * MAX_BULLETS);
	p_bullets = (Bullet*) malloc(sizeof(Bullet) * MAX_PLR_BULLETS);

	{
		Enemy* e = CreateEnemy();
		e->x = MAP_W / 2.0f + GAME_W / 2.0f;
		e->y = MAP_H / 2.0f + GAME_H / 2.0f;
		e->radius = ASTEROID_RADIUS_3;
		e->type = 3;
		e->texture = tex_asteroid3;
	}

	// prev_time = GetTime() - (1.0 / (double)GAME_FPS);
}

void Game::Quit() {
	SDL_DestroyTexture(interface_map_texture);
	SDL_DestroyTexture(interface_texture);
	SDL_DestroyTexture(fps_texture);

	free(p_bullets);
	free(bullets);
	free(enemies);

	TTF_CloseFont(fnt_mincho);

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

template <typename Obj>
static Obj* find_closest(Obj* objects, int object_count, float x, float y, float* rel_x, float* rel_y, float* out_dist) {
	Obj* result = nullptr;
	float dist_sq = INFINITY;

	for (int i = 0; i < object_count; i++) {
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

static void DrawTextureCentered(SDL_Texture* texture, float x, float y, float angle = 0.0f) {
	int w;
	int h;
	SDL_QueryTexture(texture, nullptr, nullptr, &w, &h);

	auto draw = [x, y, w, h, texture, angle](float xoff, float yoff) {
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

static Enemy* make_asteroid(float x, float y, float hsp, float vsp, int type) {
	Enemy* e = game->CreateEnemy();
	e->x = x;
	e->y = y;
	e->hsp = hsp;
	e->vsp = vsp;
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

void Game::Run() {
	while (!quit) {
		Frame();
	}
}

void Game::Frame() {
	double time = GetTime();

	double frame_end_time = time + (1.0 / (double)GAME_FPS);

	double _fps = 1.0 / (time - prev_time);
	prev_time = time;

	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
			case SDL_QUIT: {
				quit = true;
				break;
			}

			case SDL_KEYDOWN: {
				if (event.key.keysym.scancode == SDL_SCANCODE_TAB) {
					hide_interface ^= true;
				}
				break;
			}
		}
	}

	float delta = 1.0f;

	if (!hide_interface || !fps_texture) {
		fps_sum += _fps;
		fps_timer += delta;
		if (fps_timer >= 60.0f || !fps_texture) {
			double fps = fps_sum / fps_timer;

			if (fps_texture) SDL_DestroyTexture(fps_texture);

			char buf[10];
			SDL_snprintf(buf, sizeof(buf), "%f", fps);
			SDL_Surface* surf = TTF_RenderText_Blended(fnt_mincho, buf, {255, 255, 255, 255});
			
			fps_texture = SDL_CreateTextureFromSurface(renderer, surf);
			SDL_FreeSurface(surf);
			
			fps_timer = 0.0f;
			fps_sum = 0.0f;
		}
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

	// :update
	{
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
			if (Enemy* e = find_closest(enemies, enemy_count, p->x, p->y, &rel_x, &rel_y, &dist)) {
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
				if (input & INPUT_UP) {
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
				decelerate(PLAYER_ACC / 4.0f);
			}
		}

		// limit speed
		float player_spd = length(p->hsp, p->vsp);
		if (player_spd > PLAYER_MAX_SPD) {
			p->hsp = p->hsp / player_spd * PLAYER_MAX_SPD;
			p->vsp = p->vsp / player_spd * PLAYER_MAX_SPD;
		}

		// :physics

		p->x += p->hsp * delta;
		p->y += p->vsp * delta;

		if (p->x < 0.0f) {p->x += MAP_W; camera_x += MAP_W;}
		if (p->y < 0.0f) {p->y += MAP_H; camera_y += MAP_H;}
		if (p->x >= MAP_W) {p->x -= MAP_W; camera_x -= MAP_W;}
		if (p->y >= MAP_H) {p->y -= MAP_H; camera_y -= MAP_H;}

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

			if (p->x < 0.0f) p->x += MAP_W;
			if (p->y < 0.0f) p->y += MAP_H;
			if (p->x >= MAP_W) p->x -= MAP_W;
			if (p->y >= MAP_H) p->y -= MAP_H;
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

		for (int enemy_idx = 0, c = enemy_count; enemy_idx < c;) {
			Enemy* e = &enemies[enemy_idx];

			for (int pb_idx = 0; pb_idx < p_bullet_count;) {
				Bullet* pb = &p_bullets[pb_idx];

				if (circle_vs_circle(e->x, e->y, e->radius, pb->x, pb->y, pb->radius)) {
					e->hp -= pb->dmg;
					
					float dir = point_direction(pb->x, pb->y, e->x, e->y);
					dir += (float)(-20 + rand() % 40);

					DestroyPlrBullet(pb_idx);
					
					if (e->hp <= 0.0f) {
						if (e->type == 3) {
							make_asteroid(e->x, e->y, e->hsp + lengthdir_x(1.0f, dir + 90.0f), e->vsp + lengthdir_y(1.0f, dir + 90.0f), 2);
							make_asteroid(e->x, e->y, e->hsp + lengthdir_x(1.0f, dir - 90.0f), e->vsp + lengthdir_y(1.0f, dir - 90.0f), 2);
						} else if (e->type == 2) {
							make_asteroid(e->x, e->y, e->hsp + lengthdir_x(1.0f, dir + 90.0f), e->vsp + lengthdir_y(1.0f, dir + 90.0f), 1);
							make_asteroid(e->x, e->y, e->hsp + lengthdir_x(1.0f, dir - 90.0f), e->vsp + lengthdir_y(1.0f, dir - 90.0f), 1);
						}

						DestroyEnemy(enemy_idx);
						c--;
						goto enemy_continue;
					}
					
					continue;
				}

				pb_idx++;
			}
			
			enemy_idx++;

			enemy_continue:;
		}

		// :late update

		if (input_press & INPUT_FIRE) {
			Bullet* pb = CreatePlrBullet();
			pb->x = p->x;
			pb->y = p->y;

			pb->hsp = p->hsp;
			pb->vsp = p->vsp;

			pb->hsp += lengthdir_x(15.0f, p->dir);
			pb->vsp += lengthdir_y(15.0f, p->dir);
		}

		{
			constexpr float f = 1.0f - 0.4f;
			camera_x = lerp(camera_x, p->x - (float)GAME_W / 2.0f, 1.0f - SDL_powf(f, delta));
			camera_y = lerp(camera_y, p->y - (float)GAME_H / 2.0f, 1.0f - SDL_powf(f, delta));
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

		// spawn enemies
		{
			
		}
	}

	// :draw
	{
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);

		// draw bg
		auto draw_bg = [camera_x = this->camera_x, camera_y = this->camera_y, renderer = this->renderer](SDL_Texture* texture, float parallax) {
			float bg_w;
			float bg_h;
			{
				int w;
				int h;
				SDL_QueryTexture(texture, nullptr, nullptr, &w, &h);
				bg_w = (float)w;
				bg_h = (float)h;
			}
			float cam_x_para = camera_x / parallax;
			float cam_y_para = camera_y / parallax;
			
			float y = SDL_floorf(cam_y_para / bg_h) * bg_h;
			while (y < cam_y_para + (float)GAME_H) {
				float x = SDL_floorf(cam_x_para / bg_w) * bg_w;
				while (x < cam_x_para + (float)GAME_W) {
					SDL_FRect dest = {
						x - cam_x_para,
						y - cam_y_para,
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

		{
			auto draw = [](float xoff, float yoff) {
				int w;
				int h;
				SDL_QueryTexture(game->tex_moon, nullptr, nullptr, &w, &h);
				SDL_FRect dest = {
					500.0f - (game->camera_x + xoff) / 5.0f,
					500.0f - (game->camera_y + yoff) / 5.0f,
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
			DrawCircleCam(e->x, e->y, e->radius, {255, 0, 0, 255});
			DrawTextureCentered(e->texture, e->x, e->y);
		}

		// draw player
		{
			DrawTextureCentered(tex_player_ship, player.x, player.y, player.dir);
		}

		for (int i = 0; i < bullet_count; i++) {
			Bullet* b = &bullets[i];
			DrawCircleCam(b->x, b->y, b->radius);
		}

		for (int i = 0; i < p_bullet_count; i++) {
			Bullet* pb = &p_bullets[i];
			DrawCircleCam(pb->x, pb->y, pb->radius);
		}

		// draw interface
		if (!hide_interface || !interface_texture || !interface_map_texture) {
			int map_w = 100;
			int map_h = 100;
			interface_update_t -= delta;
			if (interface_update_t <= 0.0f) {
				char buf[40];
				SDL_snprintf(buf, sizeof(buf),
							 "X: %f\n"
							 "Y: %f",
							 player.x,
							 player.y);
				SDL_Surface* surf = TTF_RenderText_Blended_Wrapped(fnt_mincho, buf, {255, 255, 255, 255}, 0);
				interface_texture = SDL_CreateTextureFromSurface(renderer, surf);
				SDL_FreeSurface(surf);

				if (!interface_map_texture) interface_map_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, map_w, map_h);
				SDL_SetRenderTarget(renderer, interface_map_texture);
				{
					SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
					SDL_RenderClear(renderer);
					
					for (int i = 0; i < enemy_count; i++) {
						SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
						SDL_Rect r = {
							(int)(enemies[i].x / MAP_W * (float)map_w),
							(int)(enemies[i].y / MAP_H * (float)map_h),
							2,
							2
						};
						SDL_RenderFillRect(renderer, &r);
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

				interface_update_t = 5.0f;
			}

			int y;
			{
				SDL_Rect dest = {};
				SDL_QueryTexture(interface_texture, nullptr, nullptr, &dest.w, &dest.h);
				SDL_RenderCopy(renderer, interface_texture, nullptr, &dest);
				y = dest.h;
			}
			{
				SDL_Rect dest = {0, y, map_w, map_h};
				SDL_RenderCopy(renderer, interface_map_texture, nullptr, &dest);
			}

			// draw fps
			{
				SDL_Rect dest;
				SDL_QueryTexture(fps_texture, nullptr, nullptr, &dest.w, &dest.h);
				dest.x = GAME_W - dest.w;
				dest.y = GAME_H - dest.h;
				SDL_RenderCopy(renderer, fps_texture, nullptr, &dest);
			}
		}

		SDL_RenderPresent(renderer);
	}

#ifndef __EMSCRIPTEN__
	time = GetTime();
	double time_left = frame_end_time - time;
	if (time_left > 0.0) {
		SDL_Delay((Uint32)(time_left * 0.95));
		while (GetTime() < frame_end_time) {}
	}
#endif
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

#ifdef __EMSCRIPTEN__
static void emscripten_main_loop() {
	game->Frame();
}
#endif

int main() {
	Game game_instance = {};
	game = &game_instance;

	game->Init();

#ifdef __EMSCRIPTEN__
	emscripten_set_main_loop(emscripten_main_loop, 60, 1);
#else
	game->Run();
#endif

	game->Quit();

	return 0;
}
