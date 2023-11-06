#include "World.h"

#include "Game.h"
#include "Assets.h"
#include "Audio.h"

#include "mathh.h"
#include "ecalloc.h"
#include <string.h>

#define PLAYER_ACC 0.5f
#define PLAYER_MAX_SPD 10.0f
#define PLAYER_FOCUS_SPD 3.0f
#define PLAYER_TURN_SPD 6.0f

#define ASTEROID_RADIUS_3 50.0f
#define ASTEROID_RADIUS_2 25.0f
#define ASTEROID_RADIUS_1 12.0f

#define PAUSE_MENU_LEN 8

#define INTERFACE_MAP_W 200
#define INTERFACE_MAP_H 200

World* world;

static Enemy* make_asteroid(float x, float y, float hsp, float vsp, int enemy_type, float power = 1.0f) {
	Enemy* e = world->CreateEnemy();
	e->x = x;
	e->y = y;
	e->hsp = hsp;
	e->vsp = vsp;
	e->power = power;
	e->health = 1.0f;
	e->max_health = 1.0f;
	if (enemy_type == 3) {
		e->radius = ASTEROID_RADIUS_3;
		e->enemy_type = 3;
		e->sprite = spr_asteroid3;
	} else if (enemy_type == 2) {
		e->radius = ASTEROID_RADIUS_2;
		e->enemy_type = 2;
		e->sprite = spr_asteroid2;
	} else if (enemy_type == 1) {
		e->radius = ASTEROID_RADIUS_1;
		e->enemy_type = 1;
		e->sprite = spr_asteroid1;
	}
	return e;
}

void World::Init() {
	Player* p = &player;

	p->type = ObjType::PLAYER;
	p->id = next_id++;
	p->x = MAP_W / 2.0f;
	p->y = MAP_H / 2.0f;

	{
		camera_base_x = p->x;
		camera_base_y = p->y;

		camera_x = camera_base_x;
		camera_y = camera_base_y;

		camera_left = camera_x - camera_w / 2.0f;
		camera_top  = camera_y - camera_h / 2.0f;
	}

	enemies   = (Enemy*)  ecalloc(MAX_ENEMIES,     sizeof(*enemies));
	bullets   = (Bullet*) ecalloc(MAX_BULLETS,     sizeof(*bullets));
	p_bullets = (Bullet*) ecalloc(MAX_PLR_BULLETS, sizeof(*p_bullets));
	allies    = (Ally*)   ecalloc(MAX_ALLIES,      sizeof(*allies));

	particles.Init();

	{
		// Spawn Asteroids.
		int i = 200;
		while (i--) {
			float x = random_range(&rng, 0.0f, MAP_W);
			float y = random_range(&rng, 0.0f, MAP_H);
			float dist = point_distance(x, y, player.x, player.y);
			if (dist < 800.0f) {
				i++;
				continue;
			}

			float dir = random_range(&rng, 0.0f, 360.0f);
			float spd = random_range(&rng, 1.0f, 3.0f);
			make_asteroid(x, y, lengthdir_x(spd, dir), lengthdir_y(spd, dir), random_range(&rng, 1, 3));
		}
	}

	extern void stage0_script(mco_coro*);
	mco_desc desc = mco_desc_init(stage0_script, 0);
	mco_create(&co, &desc);

	{
		SDL_Renderer* renderer = game->renderer;
		interface_map_texture = SDL_CreateTexture(renderer,
												  SDL_PIXELFORMAT_XRGB8888,
												  SDL_TEXTUREACCESS_TARGET,
												  INTERFACE_MAP_W, INTERFACE_MAP_H);
	}
}

void World::Quit() {
	SDL_DestroyTexture(interface_map_texture);

	mco_destroy(co);

	particles.Free();

	free(allies);
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
				*out_dist = sqrtf(dist_sq);
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
}

void World::Update(float delta) {
	{
		// Input.
		const u8* key = SDL_GetKeyboardState(nullptr);

		u32 prev = input;
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
				case 0: game->set_audio3d(!game->options.audio_3d);    break;
				case 1: game->show_debug_info         ^= true;         break;
				case 2: game->show_audio_channels     ^= true;         break;
				case 4: game->options.letterbox       ^= true;         break;
				case 5: game->options.bilinear_filter ^= true;         break;
				case 6: game->set_vsync(!game->get_vsync());           break;
				case 7: game->set_fullscreen(!game->get_fullscreen()); break;
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

	{
		Player* p = &player;

		UpdatePlayer(p, delta);
	}

	for (int i = 0; i < enemy_count; i++) {
		Enemy* e = &enemies[i];
		switch (e->enemy_type) {
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

		e->frame_index = sprite_get_next_frame_index(e->sprite, e->frame_index, delta);
	}

	PhysicsUpdate(delta);

	// :late update

	{
		Player* p = &player;
		p->fire_timer += delta;
		while (p->fire_timer >= 10.0f) {
			if (input & INPUT_FIRE) {
				if (p->fire_queue == 0) p->fire_queue = 4;
			}

			if (p->fire_queue > 0) {
				auto shoot = [=](float spd, float dir, float hoff = 0.0f) {
					Bullet* pb = CreatePlrBullet();
					pb->x = p->x;
					pb->y = p->y;
					pb->dmg = 10.0f;

					// forward offset
					pb->x += lengthdir_x(20.0f, dir);
					pb->y += lengthdir_y(20.0f, dir);

					// side offset
					pb->x += lengthdir_x(hoff, dir - 90.0f);
					pb->y += lengthdir_y(hoff, dir - 90.0f);

					pb->hsp = p->hsp + lengthdir_x(spd, dir);
					pb->vsp = p->vsp + lengthdir_y(spd, dir);

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

				play_sound(snd_shoot, p->x, p->y, 10);

				p->fire_queue--;
			}

			p->fire_timer -= 10.0f;
		}

		p->invincibility -= delta;
		if (p->invincibility < 0.0f) p->invincibility = 0.0f;

		{
			float target_x = p->x;
			float target_y = p->y;

			// Active cam.
			target_x += lengthdir_x(100.0f, p->dir);
			target_y += lengthdir_y(100.0f, p->dir);

			camera_base_x += p->hsp * delta;
			camera_base_y += p->vsp * delta;

			{
				const float f = 1.0f - 0.02f;
				camera_base_x = lerp(camera_base_x, target_x, 1.0f - powf(f, delta));
				camera_base_y = lerp(camera_base_y, target_y, 1.0f - powf(f, delta));
			}

			{
				const float f = 1.0f - 0.01f;
				camera_scale = lerp(camera_scale, camera_scale_target, 1.0f - powf(f, delta));
			}

			screenshake_timer -= delta;
			if (screenshake_timer < 0.0f) screenshake_timer = 0.0f;

			camera_x = camera_base_x;
			camera_y = camera_base_y;

			if (screenshake_timer > 0.0f) {
				if (screenshake_time >= 60.0f) {   // long
					float f = screenshake_timer / screenshake_time;
					float range = screenshake_intensity * f;
					camera_x += random_range(&rng, -range, range);
					camera_y += random_range(&rng, -range, range);
				}
				// else if (screenshake_time <= 6.0f) {  // short
				// 	camera_x += float((frame % 2) * 2 - 1) * screenshake_intensity;
				// }
				else {
					float range = screenshake_intensity;
					camera_x += random_range(&rng, -range, range);
					camera_y += random_range(&rng, -range, range);
				}
			}
		}
	}

	for (int i = 0; i < bullet_count; i++) {
		Bullet* b = &bullets[i];
		b->lifetime += delta;
		if (b->lifetime >= b->lifespan) {
			DestroyBulletByIndex(i);
			i--;
		}
	}

	for (int i = 0; i < p_bullet_count; i++) {
		Bullet* pb = &p_bullets[i];
		pb->lifetime += delta;
		if (pb->lifetime >= pb->lifespan) {
			DestroyPlrBulletByIndex(i);
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

	particles.Update(delta);

	if (game->key_pressed[SDL_SCANCODE_TAB]) {
		hide_interface ^= true;
	}
	if (game->key_pressed[SDL_SCANCODE_H]) {
		show_hitboxes ^= true;
	}

	// time += delta;

	update_interface(delta);

l_skip_update:

	if (game->key_pressed[SDL_SCANCODE_ESCAPE]) {
		paused ^= true;
		if (paused) pause_menu = {};
	}

	frame++;
}

void World::UpdatePlayer(Player* p, float delta) {
	p->focus = (input & INPUT_FOCUS) != 0;

	float max_spd = PLAYER_MAX_SPD;

	if (p->focus) {
		if (input & INPUT_RIGHT) {p->x += PLAYER_FOCUS_SPD * delta; camera_base_x += PLAYER_FOCUS_SPD * delta;}
		if (input & INPUT_UP)    {p->y -= PLAYER_FOCUS_SPD * delta; camera_base_y -= PLAYER_FOCUS_SPD * delta;}
		if (input & INPUT_LEFT)  {p->x -= PLAYER_FOCUS_SPD * delta; camera_base_x -= PLAYER_FOCUS_SPD * delta;}
		if (input & INPUT_DOWN)  {p->y += PLAYER_FOCUS_SPD * delta; camera_base_y += PLAYER_FOCUS_SPD * delta;}

		decelerate(p, 2.0f * PLAYER_ACC, delta);

		float rel_x;
		float rel_y;
		float dist;
		if (Enemy* e = find_closest(enemies, enemy_count,
									p->x, p->y,
									&rel_x, &rel_y, &dist,
									[](Enemy* e) { return e->enemy_type >= TYPE_ENEMY; })) {
			if (dist < 800.0f) {
				const float f = 1.0f - 0.05f;
				float dir = point_direction(p->x, p->y, rel_x, rel_y);
				float target = p->dir - angle_difference(p->dir, dir);
				p->dir = lerp(p->dir, target, 1.0f - powf(f, delta));
			}
		}
	} else {
		float acc = PLAYER_ACC;

		if ((input & INPUT_BOOST) && (p->boost > 0.0f)) {
			max_spd = 14.0f;
			acc *= 1.5f;
			if (input & INPUT_UP) {
				p->boost -= (1.0f / 3.0f) * delta;
				p->boost = max(p->boost, 0.0f);
			}
		}

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
			p->hsp += acc *  dcos(p->dir) * delta;
			p->vsp += acc * -dsin(p->dir) * delta;

			if (!sound_is_playing(snd_ship_engine)) {
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
	if (player_spd > max_spd) {
		p->hsp = p->hsp / player_spd * max_spd;
		p->vsp = p->vsp / player_spd * max_spd;
	}

	p->health += (1.0f / 120.0f) * delta;
	p->health = min(p->health, p->max_health);

	if (!(input & INPUT_BOOST)) {
		p->boost += (1.0f / 20.0f) * delta;
		p->boost = min(p->boost, p->max_boost);
	}

	switch (p->power_level) {
		case 1: {
			if (p->power >= 50.0f) {p->power_level++; play_sound(snd_powerup, p->x, p->y);}
			break;
		}
		case 2: {
			if (p->power >= 200.0f) {p->power_level++; play_sound(snd_powerup, p->x, p->y);}
			break;
		}
	}
}

void World::PhysicsUpdate(float delta) {
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

					DestroyBulletByIndex(i);
					i--;

					break;
				}
			}
		}

		if (p->invincibility == 0.0f) {
			for (int i = 0, c = enemy_count; i < c; i++) {
				Enemy* e = &enemies[i];

				if (circle_vs_circle(p->x, p->y, p->radius, e->x, e->y, e->radius)) {
					const float contact_damage = 15.0f;
					player_get_hit(p, contact_damage);

					float split_dir = point_direction(p->x, p->y, e->x, e->y);
					if (!enemy_get_hit(e, contact_damage, split_dir, false)) {
						DestroyEnemyByIndex(i);
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

		Player* p = &player;

		auto collide_with_bullets = [=](Bullet* bullets, int bullet_count, void (World::*destroy)(int), bool give_power = false) {
			for (int bullet_idx = 0; bullet_idx < bullet_count;) {
				Bullet* b = &bullets[bullet_idx];

				if (circle_vs_circle(e->x, e->y, e->radius, b->x, b->y, b->radius)) {
					float split_dir = point_direction(b->x, b->y, e->x, e->y);

					float dmg = b->dmg;
					(this->*destroy)(bullet_idx);
					bullet_count--;

					if (!enemy_get_hit(e, dmg, split_dir)) {
						if (give_power) {
							p->power += e->power;
						}
						return false;
					}

					continue;
				}

				bullet_idx++;
			}

			return true;
		};

		if (!collide_with_bullets(p_bullets, p_bullet_count, &World::DestroyPlrBulletByIndex, true)) {
			DestroyEnemyByIndex(enemy_idx);
			c--;
			continue;
		}

		// if (e->enemy_type < TYPE_ENEMY) {
		// 	if (!collide_with_bullets(bullets, bullet_count, &Game::DestroyBullet)) {
		// 		DestroyEnemyByIndex(enemy_idx);
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

	ScreenShake(p->x, p->y, 5.0f, 10.0f);
	play_sound(snd_hurt, p->x, p->y);
	game->sleep(p->x, p->y, 50);
}

bool World::enemy_get_hit(Enemy* e, float dmg, float split_dir, bool _play_sound) {
	e->health -= dmg;

	split_dir += random_range(&rng, -5.0f, 5.0f);

	if (_play_sound) {
		play_sound(snd_hurt, e->x, e->y);
	}

	if (e->health <= 0.0f) {
		switch (e->enemy_type) {
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
		}

		Mix_Chunk* sound = (e->enemy_type >= TYPE_BOSS) ? snd_boss_explode : snd_explode;
		play_sound(sound, e->x, e->y);

		if (e->enemy_type >= TYPE_BOSS) {
			ScreenShake(e->x, e->y, 15.0f, 60.0f, true);
			game->sleep(e->x, e->y, 100);
		} else if (e->enemy_type >= TYPE_ENEMY) {
			ScreenShake(e->x, e->y, 5.0f, 10.0f);
			game->sleep(e->x, e->y, 50);
		} else {
			ScreenShake(e->x, e->y, 2.0f, 8.0f);
			game->sleep(e->x, e->y, 20);
		}

		return false;
	}

	return true;
}

void World::ScreenShake(float x, float y, float intensity, float time, bool always) {
	if (!always) {
		if (!is_on_screen(x, y)) {
			return;
		}

		// if there already is a screen shake and it's intensity is higher
		if (screenshake_timer > 0.0f) {
			if (screenshake_intensity > intensity) {
				return;
			}
		}
	}

	screenshake_intensity = intensity;
	screenshake_time = time;
	screenshake_timer = time;
}

void DrawCircleCamWarped(float x, float y, float radius, SDL_Color color) {
	SDL_Renderer* renderer = game->renderer;

	auto draw = [=](float x, float y) {
		SDL_Rect contains = {
			(int) (x - radius - world->camera_left),
			(int) (y - radius - world->camera_top),
			(int) (radius * 2.0f),
			(int) (radius * 2.0f)
		};
		SDL_Rect screen = {0, 0, int(world->camera_w), int(world->camera_h)};

		if (!SDL_HasIntersection(&contains, &screen)) {
			return;
		}

		const int precision = 14;

		for (int i = 0; i < precision; i++) {
			float dir = float(i) / float(precision) * 360.0f;
			float dir1 = float(i + 1) / float(precision) * 360.0f;

			SDL_Vertex vertices[3] = {};
			vertices[0].position = {x, y};
			vertices[1].position = {x + lengthdir_x(radius, dir), y + lengthdir_y(radius, dir)};
			vertices[2].position = {x + lengthdir_x(radius, dir1), y + lengthdir_y(radius, dir1)};
			vertices[0].color = color;
			vertices[1].color = color;
			vertices[2].color = color;

			vertices[0].position.x -= world->camera_left;
			vertices[1].position.x -= world->camera_left;
			vertices[2].position.x -= world->camera_left;

			vertices[0].position.y -= world->camera_top;
			vertices[1].position.y -= world->camera_top;
			vertices[2].position.y -= world->camera_top;

			SDL_RenderGeometry(renderer, nullptr, vertices, ArrayLength(vertices), nullptr, 0);
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

/*
static void DrawTextureCentered(SDL_Texture* texture, float x, float y,
								float angle = 0.0f, SDL_Color col = {255, 255, 255, 255}) {
	SDL_Renderer* renderer = game->renderer;

	int w;
	int h;
	SDL_QueryTexture(texture, nullptr, nullptr, &w, &h);

	auto draw = [=](float xoff, float yoff) {
		SDL_FRect dest = {
			x - float(w / 2) + xoff - world->camera_left,
			y - float(h / 2) + yoff - world->camera_top,
			(float) w,
			(float) h
		};

		SDL_Rect idest = {(int)dest.x, (int)dest.y, (int)dest.w, (int)dest.h};
		SDL_Rect screen = {0, 0, world->camera_w, world->camera_h};
		if (!SDL_HasIntersection(&idest, &screen)) {
			return;
		}

		SDL_SetTextureColorMod(texture, col.r, col.g, col.b);
		SDL_SetTextureAlphaMod(texture, col.a);
		SDL_RenderCopyExF(renderer, texture, nullptr, &dest, (double)(-angle), nullptr, SDL_FLIP_NONE);
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
*/

static bool _is_on_screen(float x, float y) {
	x -= world->camera_left;
	y -= world->camera_top;

	float off = 100.0f;
	return (-off <= x && x < world->camera_w + off)
		&& (-off <= y && y < world->camera_h + off);
}

void DrawSpriteCamWarped(Sprite* sprite, int frame_index,
						 float x, float y,
						 float angle, float xscale, float yscale,
						 SDL_Color color) {
	auto draw = [=](float xoff, float yoff) {
		if (!_is_on_screen(x + xoff, y + yoff)) return;

		DrawSprite(sprite,
				   frame_index,
				   x + xoff - world->camera_left,
				   y + yoff - world->camera_top,
				   angle,
				   xscale,
				   yscale,
				   color);
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

void World::Draw(float delta) {
	SDL_Renderer* renderer = game->renderer;

	camera_w = float(game->camera_base_w) / camera_scale;
	camera_h = float(game->camera_base_h) / camera_scale;

	camera_left = camera_x - camera_w / 2.0f;
	camera_top  = camera_y - camera_h / 2.0f;

	{
		float xscale;
		float yscale;
		SDL_RenderGetScale(renderer, &xscale, &yscale);
		xscale *= camera_scale;
		yscale *= camera_scale;
		SDL_RenderSetScale(renderer, xscale, yscale);
	}

	{
		// Draw background.

		auto draw_bg = [=](SDL_Texture* texture, float parallax) {
			int bg_w;
			int bg_h;
			SDL_QueryTexture(texture, nullptr, nullptr, &bg_w, &bg_h);

			int bg_x = int(camera_x - camera_left - camera_x / parallax);
			while (bg_x > 0) bg_x -= bg_w;

			int bg_y = int(camera_y - camera_top - camera_y / parallax);
			while (bg_y > 0) bg_y -= bg_h;

			for (int y = bg_y; y < int(camera_h); y += bg_h) {
				for (int x = bg_x; x < int(camera_w); x += bg_w) {
					SDL_Rect dest = {x, y, bg_w, bg_h};
					SDL_RenderCopy(renderer, texture, nullptr, &dest);
				}
			}
		};

		draw_bg(tex_bg, 5.0f);

		{
			// Draw moon.

			auto draw_moon = [=](float xoff, float yoff) {
				SDL_Texture* texture = tex_moon;

				int bg_w;
				int bg_h;
				SDL_QueryTexture(texture, nullptr, nullptr, &bg_w, &bg_h);

				const float parallax = 5.0f;

				int bg_x = int(xoff / parallax + camera_x - camera_left - camera_x / parallax);
				int bg_y = int(yoff / parallax + camera_y - camera_top  - camera_y / parallax);

				SDL_Rect dest = {bg_x, bg_y, bg_w, bg_h};
				SDL_Rect screen = {0, 0, int(camera_w), int(camera_h)};
				if (!SDL_HasIntersection(&dest, &screen)) {
					return;
				}

				SDL_RenderCopy(renderer, texture, nullptr, &dest);
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

		draw_bg(tex_bg1, 4.0f);
	}

	// Draw enemies.
	for (int i = 0; i < enemy_count; i++) {
		Enemy* e = &enemies[i];
		if (show_hitboxes) DrawCircleCamWarped(e->x, e->y, e->radius, {255, 0, 0, 255});
		DrawSpriteCamWarped(e->sprite, int(e->frame_index), e->x, e->y, e->angle);
	}

	{
		// Draw player.

		SDL_Color col = {255, 255, 255, 255};
		if (player.invincibility > 0.0f) {
			if ((SDL_GetTicks() / 16 / 4) % 2) col.a = 64;
			else col.a = 192;
		}
		if (show_hitboxes) DrawCircleCamWarped(player.x, player.y, player.radius, {128, 255, 128, 255});
		DrawSpriteCamWarped(spr_player_ship, 0, player.x, player.y, player.dir, 1.0f, 1.0f, col);
	}

	// Draw bullets.
	for (int i = 0; i < bullet_count; i++) {
		Bullet* b = &bullets[i];
		DrawCircleCamWarped(b->x, b->y, b->radius);
	}

	// Draw player bullets.
	for (int i = 0; i < p_bullet_count; i++) {
		Bullet* pb = &p_bullets[i];
		DrawCircleCamWarped(pb->x, pb->y, pb->radius);
	}

	particles.Draw(delta);
}

void World::DrawUI(float delta) {
	SDL_Renderer* renderer = game->renderer;

	if (!hide_interface) {
		draw_ui(delta);
	} else {
		interface_update_timer = 0.0f;
	}

	if (paused) {
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 128);
		SDL_RenderFillRect(renderer, nullptr);

		char label4[20];
		stb_snprintf(label4, sizeof(label4), "SOUND VOLUME: %d", Mix_Volume(0, -1));

		const char* label[PAUSE_MENU_LEN] = {
			game->options.audio_3d        ? "3D AUDIO (experimental): on" : "3D AUDIO (experimental): off",
			game->show_debug_info         ? "SHOW DEBUG INFO: on"         : "SHOW DEBUG INFO: off",
			game->show_audio_channels     ? "SHOW AUDIO CHANNELS: on"     : "SHOW AUDIO CHANNELS: off",
			label4,
			game->options.letterbox       ? "LETTERBOX: on"               : "LETTERBOX: off",
			game->options.bilinear_filter ? "BILINEAR FILTERING: on"      : "BILINEAR FILTERING: off",
			game->get_vsync()             ? "VSYNC: on"                   : "VSYNC: off",
			game->get_fullscreen()        ? "FULLSCREEN: on"              : "FULLSCREEN: off"
		};

		for (int i = 0; i < PAUSE_MENU_LEN; i++) {
			int x = game->ui_w / 2;
			int y = 100 + 22 * i;
			SDL_Color col = (i == pause_menu.cursor) ? SDL_Color{255, 255, 0, 255} : SDL_Color{255, 255, 255, 255};
			DrawText(renderer, fnt_mincho, label[i], x, y, HALIGN_CENTER, 0, col);
		}
	}
}

static void draw_bar(int x, int y, int w, int h, float amount,
					 SDL_Color fill_col,
					 SDL_Color back_col = {0, 0, 0, 255},
					 SDL_Color outline_col = {1, 2, 3, 0}) {
	SDL_Renderer* renderer = game->renderer;

	SDL_Rect back = {x, y, w, h};
	SDL_SetRenderDrawColor(renderer, back_col.r, back_col.g, back_col.b, back_col.a);
	SDL_RenderFillRect(renderer, &back);
	int filled_w = (int) (amount * float(w));
	SDL_Rect filled = {back.x, back.y, filled_w, back.h};
	SDL_SetRenderDrawColor(renderer, fill_col.r, fill_col.g, fill_col.b, fill_col.a);
	SDL_RenderFillRect(renderer, &filled);

	// Outline.
	if ((outline_col.r == 1)
		&& (outline_col.g == 2)
		&& (outline_col.b == 3)
		&& (outline_col.a == 0)) {
		outline_col = fill_col;
	}
	if (outline_col.a > 0) {
		SDL_SetRenderDrawColor(renderer, outline_col.r, outline_col.g, outline_col.b, outline_col.a);
		SDL_RenderDrawRect(renderer, &back);
	}
}

void World::draw_ui(float delta) {
	// :ui

	SDL_Renderer* renderer = game->renderer;

	int x = 10;
	int y = 10;

	{
		// Draw map.

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
		if (src.x > INTERFACE_MAP_W - w) src.x = INTERFACE_MAP_W - w;
		if (src.y > INTERFACE_MAP_H - h) src.y = INTERFACE_MAP_H - h;
		SDL_Rect dest = {x, y, w, h};
		SDL_RenderCopy(renderer, interface_map_texture, &src, &dest);

		// Outline.
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
		SDL_RenderDrawRect(renderer, &dest);

		y += h;
		y += 8;
	}
	{
		// Draw healthbar.
		int w = 180;
		int h = 18;
		draw_bar(x, y, w, h, player.health / player.max_health, {255, 0, 0, 255});

		char buf[32];
		stb_snprintf(buf, sizeof(buf), "HEALTH %.0f/%.0f", player.health, player.max_health);
		DrawText(renderer, fnt_cp437, buf, x + w / 2, y + h / 2, HALIGN_CENTER, VALIGN_MIDDLE);

		y += h;
		y += 4;
	}
	{
		// Draw boost meter.
		int w = 180;
		int h = 18;
		draw_bar(x, y, w, h, player.boost / player.max_boost, {0, 0, 255, 255});

		char buf[32];
		stb_snprintf(buf, sizeof(buf), "BOOST %.0f/%.0f", player.boost, player.max_boost);
		DrawText(renderer, fnt_cp437, buf, x + w / 2, y + h / 2, HALIGN_CENTER, VALIGN_MIDDLE);

		y += h;
		y += 8;
	}
	{
		y -= 2;
		char buf[100];
		stb_snprintf(buf, sizeof(buf),
					 "X: %f\n"
					 "Y: %f\n"
					 "POWER: %.2f\n",
					 interface_x,
					 interface_y,
					 player.power);
		y = DrawText(renderer, fnt_cp437, buf, x, y).y;
	}

	// Draw boss healthbar.
	for (int i = 0; i < enemy_count; i++) {
		if (enemies[i].enemy_type < TYPE_BOSS) continue;

		int w = 450;
		int h = 18;
		SDL_Rect back = {(game->ui_w - w) / 2, 20, w, h};
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderFillRect(renderer, &back);
		int filled_w = (int) (enemies[i].health / enemies[i].max_health * (float)w);
		SDL_Rect filled = {back.x, back.y, filled_w, back.h};
		SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
		SDL_RenderFillRect(renderer, &filled);

		// Outline.
		SDL_RenderDrawRect(renderer, &back);

		char buf[32];
		stb_snprintf(buf, sizeof(buf), "BOSS %.0f/%.0f", enemies[i].health, enemies[i].max_health);
		DrawText(renderer, fnt_cp437, buf, back.x + w / 2, back.y + h / 2, HALIGN_CENTER, VALIGN_MIDDLE);

		break;
	}
}

void World::update_interface(float delta) {
	SDL_Renderer* renderer = game->renderer;

	interface_update_timer -= delta;
	if (interface_update_timer <= 0.0f) {
		interface_x = player.x;
		interface_y = player.y;

		SDL_SetRenderTarget(renderer, interface_map_texture);
		{
			SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
			SDL_RenderClear(renderer);

			for (int i = 0; i < enemy_count; i++) {
				SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
				if (enemies[i].enemy_type < TYPE_ENEMY) {
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

		interface_update_timer = 5.0f;
	}
}

Enemy* World::CreateEnemy() {
	if (enemy_count == MAX_ENEMIES) {
		SDL_Log("Enemy limit hit.");
		DestroyEnemyByIndex(0);
	}

	Enemy* result = &enemies[enemy_count];
	*result = {};
	result->type = ObjType::ENEMY;
	result->id = next_id++;
	enemy_count++;

	return result;
}

Bullet* World::CreateBullet() {
	if (bullet_count == MAX_BULLETS) {
		SDL_Log("Bullet limit hit.");
		DestroyBulletByIndex(0);
	}

	Bullet* result = &bullets[bullet_count];
	*result = {};
	result->type = ObjType::BULLET;
	result->id = next_id++;
	bullet_count++;

	return result;
}

Bullet* World::CreatePlrBullet() {
	if (p_bullet_count == MAX_PLR_BULLETS) {
		SDL_Log("Player bullets limit hit.");
		DestroyPlrBulletByIndex(0);
	}

	Bullet* result = &p_bullets[p_bullet_count];
	*result = {};
	result->type = ObjType::PLAYER_BULLET;
	result->id = next_id++;
	p_bullet_count++;

	return result;
}

void World::DestroyEnemyByIndex(int enemy_idx) {
	if (enemy_count == 0) {
		return;
	}

	if (enemies[enemy_idx].co) {
		mco_destroy(enemies[enemy_idx].co);
	}

	for (int i = enemy_idx + 1; i < enemy_count; i++) {
		enemies[i - 1] = enemies[i];
	}
	enemy_count--;
}

void World::DestroyBulletByIndex(int bullet_idx) {
	if (bullet_count == 0) {
		return;
	}

	for (int i = bullet_idx + 1; i < bullet_count; i++) {
		bullets[i - 1] = bullets[i];
	}
	bullet_count--;
}

void World::DestroyPlrBulletByIndex(int p_bullet_idx) {
	if (p_bullet_count == 0) {
		return;
	}

	for (int i = p_bullet_idx + 1; i < p_bullet_count; i++) {
		p_bullets[i - 1] = p_bullets[i];
	}
	p_bullet_count--;
}
