#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

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

	float t;
};

struct Bullet {
	float x;
	float y;
	float hsp;
	float vsp;
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

	SDL_Window* window;
	SDL_Renderer* renderer;
	double prev_time;
	bool quit;

	void Init();
	void Quit();
	void Run();
	void Frame();

	Enemy* CreateEnemy();
	Bullet* CreateBullet();
	Bullet* CreatePlrBullet();
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

void Game::Init() {
	SDL_Init(SDL_INIT_VIDEO);
	IMG_Init(IMG_INIT_PNG);

	window = SDL_CreateWindow("04Asteroids",
							  SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
							  GAME_W, GAME_H,
							  SDL_WINDOW_RESIZABLE);

	renderer = SDL_CreateRenderer(window, -1,
								  SDL_RENDERER_ACCELERATED
								  | SDL_RENDERER_TARGETTEXTURE);

	SDL_RenderSetLogicalSize(renderer, GAME_W, GAME_H);

	tex_player_ship = IMG_LoadTexture(renderer, "assets/player_ship.png");
	tex_bg = IMG_LoadTexture(renderer, "assets/bg.png");

	player.x = (float)GAME_W / 2.0f;
	player.y = (float)GAME_H / 2.0f;

	camera_x = player.x - (float)GAME_W / 2.0f;
	camera_y = player.y - (float)GAME_H / 2.0f;

	enemies   = (Enemy*)  malloc(sizeof(Enemy)  * MAX_ENEMIES);
	bullets   = (Bullet*) malloc(sizeof(Bullet) * MAX_BULLETS);
	p_bullets = (Bullet*) malloc(sizeof(Bullet) * MAX_PLR_BULLETS);

	{
		Enemy* e = CreateEnemy();
		e->x = GAME_W;
		e->y = GAME_H;
	}
}

void Game::Quit() {
	free(p_bullets);
	free(bullets);
	free(enemies);

	SDL_DestroyTexture(tex_player_ship);
	SDL_DestroyTexture(tex_bg);

	SDL_DestroyWindow(window);
	SDL_DestroyRenderer(renderer);

	IMG_Quit();
	SDL_Quit();
}

static double GetTime() {
	return (double)SDL_GetPerformanceCounter() / (double)SDL_GetPerformanceFrequency();
}

static float lerp(float a, float b, float f) {
	return a + (b - a) * f;
}

static float length(float x, float y) {
	return SDL_sqrtf(x * x + y * y);
}

static float to_radians(float deg) {
	return deg / 180.0f * M_PI;
}

static float to_degrees(float rad) {
	return rad / M_PI * 180.0f;
}

static float lengthdir_x(float len, float dir) {
	return SDL_cosf(to_radians(dir)) * len;
}

static float lengthdir_y(float len, float dir) {
	return -SDL_sinf(to_radians(dir)) * len;
}

static float angle_wrap(float deg) {
	deg = SDL_fmodf(deg, 360.0f);
	if (deg < 0.0f) {
		deg += 360.0f;
	}
	return deg;
}

static float angle_difference(float dest, float src) {
	float res = dest - src;
	res = angle_wrap(res + 180.0f) - 180.0f;
	return res;
}

static void normalize0(float x, float y, float* out_x, float* out_y) {
	float l = length(x, y);
	if (l == 0.0f) {
		*out_x = 0.0f;
		*out_y = 0.0f;
	} else {
		*out_x = x / l;
		*out_y = y / l;
	}
}

static void DrawCircle(float x, float y, float radius) {
	constexpr int precision = 12;

	for (int i = 0; i < precision; i++) {
		float dir = (float)i / (float)precision * 360.0f;
		float dir1 = (float)(i + 1) / (float)precision * 360.0f;

		SDL_Vertex vertices[3] = {};
		vertices[0].position = {x, y};
		vertices[1].position = {x + lengthdir_x(radius, dir), y + lengthdir_y(radius, dir)};
		vertices[2].position = {x + lengthdir_x(radius, dir1), y + lengthdir_y(radius, dir1)};
		vertices[0].color = {255, 255, 255, 255};
		vertices[1].color = {255, 255, 255, 255};
		vertices[2].color = {255, 255, 255, 255};

		SDL_RenderGeometry(game->renderer, nullptr, vertices, ArrayLength(vertices), nullptr, 0);
	}
}

void Game::Run() {
	while (!quit) {
		Frame();
	}
}

void Game::Frame() {
	double time = GetTime();

	double frame_end_time = time + (1.0 / (double)GAME_FPS);

	double fps = 1.0 / (time - prev_time);
	prev_time = time;

	//SDL_Log("%f", fps);

	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
			case SDL_QUIT: {
				quit = true;
				break;
			}
		}
	}

	float delta = 1.0f;

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

	// update
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

		p->x += p->hsp * delta;
		p->y += p->vsp * delta;

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

		for (int i = 0; i < enemy_count; i++) {
			Enemy* e = &enemies[i];
			e->t += delta;
			if (e->t > 60.0f) {
				Bullet* b = CreateBullet();
				b->x = e->x;
				b->y = e->y;

				// b->hsp = p->hsp;
				// b->vsp = p->vsp;
				
				float dx = p->x - e->x;
				float dy = p->y - e->y;
				float l = length(dx, dy);
				if (l != 0.0f) {
					dx /= l;
					dy /= l;
				}
				b->hsp += dx * 2.0f;
				b->vsp += dy * 2.0f;

				e->t = 0.0f;
			}
		}

		for (int i = 0; i < bullet_count; i++) {
			Bullet* b = &bullets[i];
			b->x += b->hsp * delta;
			b->y += b->vsp * delta;
		}

		for (int i = 0; i < p_bullet_count; i++) {
			Bullet* pb = &p_bullets[i];
			pb->x += pb->hsp * delta;
			pb->y += pb->vsp * delta;
		}
	}

	// draw
	{
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);

		// draw bg
		{
			float bg_w = 540.0f;
			float bg_h = 360.0f;
			float parallax = 1.0f;
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
					SDL_RenderCopyF(renderer, tex_bg, nullptr, &dest);

					x += bg_w;
				}

				y += bg_h;
			}
		}

		// draw player
		{
			SDL_Texture* texture = tex_player_ship;
			SDL_FRect dest = {player.x - 48.0f / 2.0f - camera_x, player.y - 48.0f / 2.0f - camera_y, 48.0f, 48.0f};
			SDL_RenderCopyExF(renderer, texture, nullptr, &dest, (double)(-player.dir), nullptr, SDL_FLIP_NONE);
		}

		// draw enemies
		for (int i = 0; i < enemy_count; i++) {
			Enemy* e = &enemies[i];
			DrawCircle(e->x - camera_x, e->y - camera_y, 20.0f);
		}

		for (int i = 0; i < bullet_count; i++) {
			Bullet* b = &bullets[i];
			DrawCircle(b->x - camera_x, b->y - camera_y, 5.0f);
		}

		for (int i = 0; i < p_bullet_count; i++) {
			Bullet* pb = &p_bullets[i];
			DrawCircle(pb->x - camera_x, pb->y - camera_y, 5.0f);
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
