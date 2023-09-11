#include "World.h"

#include "Game.h"

#include "mathh.h"
#include "misc.h"

#include <stdio.h>

// #define PLAYER_ACC 0.3f
#define PLAYER_ACC 0.5f
#define PLAYER_MAX_SPD 10.0f
#define PLAYER_FOCUS_SPD 3.0f
// #define PLAYER_TURN_SPD 4.0f
#define PLAYER_TURN_SPD 6.0f

#define MAX_ENEMIES 1000
#define MAX_BULLETS 1000
#define MAX_PLR_BULLETS 1000

#define ASTEROID_RADIUS_3 50.0f
#define ASTEROID_RADIUS_2 25.0f
#define ASTEROID_RADIUS_1 12.0f

#define INTERFACE_MAP_W 200
#define INTERFACE_MAP_H 200

#define PAUSE_MENU_LEN 6

enum {
	TYPE_ENEMY = 1000,
	TYPE_BOSS  = 2000
};

#include "scripts.h"

static Enemy* make_asteroid(float x, float y, float hsp, float vsp, int type, float power = 1.0f) {
	Enemy* e = world->CreateEnemy();
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

void World::Init() {
	player.x = (float)MAP_W / 2.0f;
	player.y = (float)MAP_H / 2.0f;

	camera_base_x = player.x - (float)GAME_W / 2.0f;
	camera_base_y = player.y - (float)GAME_H / 2.0f;

	enemies   = (Enemy*)  malloc(sizeof(Enemy)  * MAX_ENEMIES);
	bullets   = (Bullet*) malloc(sizeof(Bullet) * MAX_BULLETS);
	p_bullets = (Bullet*) malloc(sizeof(Bullet) * MAX_PLR_BULLETS);

	{
		// spawn asteroids

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

	interface_map_texture = SDL_CreateTexture(game->renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, INTERFACE_MAP_W, INTERFACE_MAP_H);
}

void World::Quit() {
	SDL_DestroyTexture(interface_map_texture);

	mco_destroy(co);

	free(p_bullets);
	free(bullets);
	free(enemies);
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

void World::update(float delta) {
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

	if (paused) {
		if (input_press & INPUT_UP)   {pause_menu.cursor--; pause_menu.cursor = wrap(pause_menu.cursor, PAUSE_MENU_LEN);}
		if (input_press & INPUT_DOWN) {pause_menu.cursor++; pause_menu.cursor = wrap(pause_menu.cursor, PAUSE_MENU_LEN);}

		if (input_press & INPUT_FIRE) {
			switch (pause_menu.cursor) {
				case 0: {
					game->audio_3d ^= true;
					// @Hack
					int vol = Mix_Volume(0, -1);
					Mix_AllocateChannels(0);
					Mix_AllocateChannels(MY_MIX_CHANNELS);
					Mix_Volume(-1, vol);
					SDL_memset(channel_when_played, 0, sizeof(channel_when_played));
					SDL_memset(channel_priority, 0, sizeof(channel_priority));
					break;
				}
				case 1: game->show_debug_info     ^= true; break;
				case 2: game->show_audio_channels ^= true; break;
				case 4: game->letterbox           ^= true; break;
				case 5: {
					game->bilinear_filter ^= true;
					SDL_SetTextureScaleMode(game->game_texture, game->bilinear_filter ? SDL_ScaleModeLinear : SDL_ScaleModeNearest);
					break;
				}
			}
		}

		switch (pause_menu.cursor) {
			case 3: {
				if (input_press & INPUT_LEFT) {
					int vol = Mix_Volume(0, -1);
					Mix_Volume(-1, max(vol - 8, 0));
				}
				if (input_press & INPUT_RIGHT) {
					int vol = Mix_Volume(0, -1);
					Mix_Volume(-1, min(vol + 8, MIX_MAX_VOLUME));
				}
				break;
			}
		}

		goto l_skip_update;
	}

	update_player(delta);

	for (int i = 0; i < enemy_count; i++) {
		Enemy* e = &enemies[i];
		switch (e->type) {
			case TYPE_ENEMY: {
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
					if (e->stop_when_close_to_player && dist < 200.0f && length(p->hsp, p->vsp) < 5.0f) {
						decelerate(e, 0.1f, delta);

						// e->angle -= clamp(angle_difference(e->angle, dir), ) * delta;
						e->angle = approach(e->angle, e->angle - angle_difference(e->angle, dir), 5.0f * delta);
					} else {
						if (!e->not_exact_player_dir || SDL_fabsf(angle_difference(e->angle, dir)) > 20.0f) {
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
				break;
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

				play_sound(game->snd_shoot, p->x, p->y, 10);

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

			camera_base_x += p->hsp * delta;
			camera_base_y += p->vsp * delta;

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

	coro_timer += delta;
	while (coro_timer >= 1.0f) {
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

		coro_timer -= 1.0f;
	}

	if (game->key_pressed[SDL_SCANCODE_TAB]) {
		hide_interface ^= true;
	}
	if (game->key_pressed[SDL_SCANCODE_H]) {
		show_hitboxes ^= true;
	}

	time += delta;

	l_skip_update:

	if (game->key_pressed[SDL_SCANCODE_ESCAPE]) {
		paused ^= true;
		if (paused) pause_menu = {};
	}
}

void World::update_player(float delta) {
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
									[](Enemy* e) { return e->type >= TYPE_ENEMY; })) {
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

			if (!sound_playing(game->snd_ship_engine)) {
				play_sound(game->snd_ship_engine, p->x, p->y);
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

void World::physics_update(float delta) {
	{
		Player* p = &player;
		p->x += p->hsp * delta;
		p->y += p->vsp * delta;

		if (p->x < 0.0f)   {p->x += MAP_W; camera_base_x += MAP_W;}
		if (p->y < 0.0f)   {p->y += MAP_H; camera_base_y += MAP_H;}
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

				if (circle_vs_circle(p->x, p->y, p->radius, e->x, e->y, e->radius)) {
					player_get_hit(p, 10.0f);
					
					float split_dir = point_direction(p->x, p->y, e->x, e->y);
					if (!enemy_get_hit(e, 10.0f, split_dir, false)) {
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

		auto collide_with_bullets = [this, e](Bullet* bullets, int bullet_count, void (World::*destroy)(int), bool give_power = false) {
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
								if (player.power_level < 3) {player.power_level = 3; play_sound(game->snd_powerup, player.x, player.y);}
							} else if (player.power >= 50.0f) {
								if (player.power_level < 2) {player.power_level = 2; play_sound(game->snd_powerup, player.x, player.y);}
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

		if (!collide_with_bullets(p_bullets, p_bullet_count, &World::DestroyPlrBullet, true)) {
			DestroyEnemy(enemy_idx);
			c--;
			continue;
		}

		// if (e->type < TYPE_ENEMY) {
		// 	if (!collide_with_bullets(bullets, bullet_count, &Game::DestroyBullet)) {
		// 		DestroyEnemy(enemy_idx);
		// 		c--;
		// 		continue;
		// 	}
		// }
			
		enemy_idx++;
	}
}

void World::player_get_hit(Player* p, float dmg) {
	p->health -= dmg;
	p->invincibility = 60.0f;

	screenshake_intensity = 5.0f;
	screenshake_time = 10.0f;
	screenshake_timer = 10.0f;
	SDL_Delay(40);

	play_sound(game->snd_hurt, p->x, p->y);
}

bool World::enemy_get_hit(Enemy* e, float dmg, float split_dir, bool _play_sound) {
	e->health -= dmg;

	split_dir += random.range(-5.0f, 5.0f);

	if (_play_sound) {
		play_sound(game->snd_hurt, e->x, e->y);
	}

	if (e->health <= 0.0f) {
		switch (e->type) {
			case 2: {
				make_asteroid(e->x, e->y, e->hsp + lengthdir_x(1.0f, split_dir + 90.0f), e->vsp + lengthdir_y(1.0f, split_dir + 90.0f), 1, e->power / 2.0f);
				make_asteroid(e->x, e->y, e->hsp + lengthdir_x(1.0f, split_dir - 90.0f), e->vsp + lengthdir_y(1.0f, split_dir - 90.0f), 1, e->power / 2.0f);
				break;
			}
			case 3: {
				make_asteroid(e->x, e->y, e->hsp + lengthdir_x(1.0f, split_dir + 90.0f), e->vsp + lengthdir_y(1.0f, split_dir + 90.0f), 2, e->power / 2.0f);
				make_asteroid(e->x, e->y, e->hsp + lengthdir_x(1.0f, split_dir - 90.0f), e->vsp + lengthdir_y(1.0f, split_dir - 90.0f), 2, e->power / 2.0f);
				break;
			}
			case TYPE_ENEMY: {
				screenshake_intensity = 5.0f;
				screenshake_time = 10.0f;
				screenshake_timer = 10.0f;
				SDL_Delay(40);
				break;
			}
			case TYPE_BOSS: {
				screenshake_intensity = 10.0f;
				screenshake_time = 30.0f;
				screenshake_timer = 30.0f;
				SDL_Delay(50);
				break;
			}
		}

		Mix_Chunk* chunk = (e->type >= TYPE_BOSS) ? game->snd_boss_explode : game->snd_explode;
		play_sound(chunk, e->x, e->y);

		return false;
	}

	return true;
}

static void DrawCircleCam(float x, float y, float radius, SDL_Color color = {255, 255, 255, 255}) {
	auto draw = [radius, color](float x, float y) {
		SDL_Rect contains = {(int)(x - radius - world->camera_x), (int)(y - radius - world->camera_y), (int)(radius * 2.0f), (int)(radius * 2.0f)};
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

			vertices[0].position.x -= world->camera_x;
			vertices[1].position.x -= world->camera_x;
			vertices[2].position.x -= world->camera_x;

			vertices[0].position.y -= world->camera_y;
			vertices[1].position.y -= world->camera_y;
			vertices[2].position.y -= world->camera_y;

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

static void DrawTextureCentered(SDL_Texture* texture, float x, float y,
								float angle = 0.0f, SDL_Color col = {255, 255, 255, 255}) {
	int w;
	int h;
	SDL_QueryTexture(texture, nullptr, nullptr, &w, &h);

	auto draw = [x, y, w, h, texture, angle, col](float xoff, float yoff) {
		SDL_FRect dest = {
			x - (float)w / 2.0f - world->camera_x + xoff,
			y - (float)h / 2.0f - world->camera_y + yoff,
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
		float xx = x - world->camera_x;
		float yy = y - world->camera_y;

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

void World::draw(float delta) {
	// draw bg
	{
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
					SDL_RenderCopyF(game->renderer, texture, nullptr, &dest);

					x += bg_w;
				}

				y += bg_h;
			}
		};

		draw_bg(game->tex_bg, 5.0f);

		// draw moon
		{
			auto draw_moon = [this](float xoff, float yoff) {
				int w;
				int h;
				SDL_QueryTexture(game->tex_moon, nullptr, nullptr, &w, &h);
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
		
				SDL_RenderCopyF(game->renderer, game->tex_moon, nullptr, &dest);
			};
		
			draw_moon(-MAP_W, -MAP_H);
			draw_moon( 0.0f,  -MAP_H);
			draw_moon( MAP_W, -MAP_H);
		
			draw_moon(-MAP_W,  0.0f);
			draw_moon( 0.0f,   0.0f);
			draw_moon( MAP_W,  0.0f);
		
			draw_moon(-MAP_W,  MAP_H);
			draw_moon( 0.0f,   MAP_H);
			draw_moon( MAP_W,  MAP_H);
		}

		draw_bg(game->tex_bg1, 4.0f);
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
		DrawSpriteCamWarped(&game->spr_player_ship, 0.0f, player.x, player.y, player.dir, 1.0f, 1.0f, col);
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

	if (paused) {
		SDL_SetRenderDrawColor(game->renderer, 0, 0, 0, 128);
		SDL_RenderFillRect(game->renderer, nullptr);

		char label4[20];
		SDL_snprintf(label4, sizeof(label4), "SOUND VOLUME: %d", Mix_Volume(0, -1));

		const char* label[PAUSE_MENU_LEN] = {
			game->audio_3d            ? "3D AUDIO (experimental): on" : "3D AUDIO (experimental): off",
			game->show_debug_info     ? "SHOW DEBUG INFO: on"         : "SHOW DEBUG INFO: off",
			game->show_audio_channels ? "SHOW AUDIO CHANNELS: on"     : "SHOW AUDIO CHANNELS: off",
			label4,
			game->letterbox           ? "LETTERBOX: on"               : "LETTERBOX: off",
			game->bilinear_filter     ? "BILINEAR FILTERING: on"      : "BILINEAR FILTERING: off"
		};

		for (int i = 0; i < PAUSE_MENU_LEN; i++) {
			int x = GAME_W / 2;
			int y = 100 + 22 * i;
			SDL_Color col = (i == pause_menu.cursor) ? SDL_Color{255, 255, 0, 255} : SDL_Color{255, 255, 255, 255};
			DrawText(&game->fnt_mincho, label[i], x, y, HALIGN_CENTER, 0, col);
		}
	}
}

void World::draw_ui(float delta) {
	// :ui

	interface_update_timer -= delta;
	if (interface_update_timer <= 0.0f) {
		interface_x = player.x;
		interface_y = player.y;

		SDL_Texture* old_target = SDL_GetRenderTarget(game->renderer);
		SDL_SetRenderTarget(game->renderer, interface_map_texture);
		{
			SDL_SetRenderDrawColor(game->renderer, 0, 0, 0, 255);
			SDL_RenderClear(game->renderer);

			for (int i = 0; i < enemy_count; i++) {
				SDL_SetRenderDrawColor(game->renderer, 255, 0, 0, 255);
				if (enemies[i].type < TYPE_ENEMY) {
					SDL_RenderDrawPoint(game->renderer,
										(int) (enemies[i].x / MAP_W * (float)INTERFACE_MAP_W),
										(int) (enemies[i].y / MAP_H * (float)INTERFACE_MAP_H));
				} else {
					SDL_Rect r = {
						(int) (enemies[i].x / MAP_W * (float)INTERFACE_MAP_W),
						(int) (enemies[i].y / MAP_H * (float)INTERFACE_MAP_H),
						2,
						2
					};
					SDL_RenderFillRect(game->renderer, &r);
				}
			}

			SDL_SetRenderDrawColor(game->renderer, 255, 255, 255, 255);
			SDL_Rect r = {
				(int) (player.x / MAP_W * (float)INTERFACE_MAP_W),
				(int) (player.y / MAP_H * (float)INTERFACE_MAP_H),
				2,
				2
			};
			SDL_RenderFillRect(game->renderer, &r);
		}
		SDL_SetRenderTarget(game->renderer, old_target);

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
		y += DrawText(&game->fnt_mincho, buf, x, y).y + game->fnt_mincho.height;
	}
	{
		int w = 180;
		int h = 22;
		SDL_Rect back = {x + 5, y + 5, w, h};
		SDL_SetRenderDrawColor(game->renderer, 0, 0, 0, 255);
		SDL_RenderFillRect(game->renderer, &back);
		int filled_w = (int) (player.health / player.max_health * (float)w);
		SDL_Rect filled = {back.x, back.y, filled_w, back.h};
		SDL_SetRenderDrawColor(game->renderer, 255, 0, 0, 255);
		SDL_RenderFillRect(game->renderer, &filled);

		char buf[10];
		SDL_snprintf(buf, sizeof(buf), "%.0f/%.0f", player.health, player.max_health);
		DrawText(&game->fnt_mincho, buf, back.x + w / 2, back.y + h / 2, HALIGN_CENTER, VALIGN_MIDDLE);

		y += h + 5 * 2;
	}
	{
		// SDL_Rect dest = {0, y, INTERFACE_MAP_W, INTERFACE_MAP_H};
		// SDL_RenderCopy(game->renderer, interface_map_texture, nullptr, &dest);

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
		SDL_RenderCopy(game->renderer, interface_map_texture, &src, &dest);

		y += h + 5 * 2;
	}

	// draw boss healthbar
	for (int i = 0; i < enemy_count; i++) {
		if (enemies[i].type < TYPE_BOSS) continue;

		int w = 450;
		int h = 18;
		SDL_Rect back = {(GAME_W - w) / 2, 20, w, h};
		SDL_SetRenderDrawColor(game->renderer, 0, 0, 0, 255);
		SDL_RenderFillRect(game->renderer, &back);
		int filled_w = (int) (enemies[i].health / enemies[i].max_health * (float)w);
		SDL_Rect filled = {back.x, back.y, filled_w, back.h};
		SDL_SetRenderDrawColor(game->renderer, 255, 0, 0, 255);
		SDL_RenderFillRect(game->renderer, &filled);

		char buf[10];
		SDL_snprintf(buf, sizeof(buf), "%.0f/%.0f", enemies[i].health, enemies[i].max_health);
		DrawText(&game->fnt_mincho, buf, back.x + w / 2, back.y + h / 2, HALIGN_CENTER, VALIGN_MIDDLE);

		break;
	}
}

Enemy* World::CreateEnemy() {
	if (enemy_count == MAX_ENEMIES) {
		printf("enemy limit hit\n");
		// enemy_count--;
		DestroyEnemy(0);
	}

	Enemy* result = &enemies[enemy_count];
	*result = {};
	enemy_count++;
	
	return result;
}

Bullet* World::CreateBullet() {
	if (bullet_count == MAX_BULLETS) {
		printf("bullet limit hit\n");
		// bullet_count--;
		DestroyBullet(0);
	}

	Bullet* result = &bullets[bullet_count];
	*result = {};
	bullet_count++;
	
	return result;
}

Bullet* World::CreatePlrBullet() {
	if (p_bullet_count == MAX_PLR_BULLETS) {
		printf("plr bullet limit hit\n");
		// p_bullet_count--;
		DestroyPlrBullet(0);
	}

	Bullet* result = &p_bullets[p_bullet_count];
	*result = {};
	p_bullet_count++;
	
	return result;
}

void World::DestroyEnemy(int enemy_idx) {
	if (enemies[enemy_idx].co) {
		mco_destroy(enemies[enemy_idx].co);
	}

	for (int i = enemy_idx + 1; i < enemy_count; i++) {
		enemies[i - 1] = enemies[i];
	}
	enemy_count--;
}

void World::DestroyBullet(int bullet_idx) {
	for (int i = bullet_idx + 1; i < bullet_count; i++) {
		bullets[i - 1] = bullets[i];
	}
	bullet_count--;
}

void World::DestroyPlrBullet(int p_bullet_idx) {
	for (int i = p_bullet_idx + 1; i < p_bullet_count; i++) {
		p_bullets[i - 1] = p_bullets[i];
	}
	p_bullet_count--;
}
