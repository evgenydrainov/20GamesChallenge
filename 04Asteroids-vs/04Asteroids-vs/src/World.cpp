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

#define ACTIVE_ITEM_COOLDOWN (60.0f * 60.0f)

#define ASTEROID_RADIUS_3 50.0f
#define ASTEROID_RADIUS_2 25.0f
#define ASTEROID_RADIUS_1 12.0f

#define PAUSE_MENU_LEN 8

#define INTERFACE_MAP_W 200
#define INTERFACE_MAP_H 200

enum {
	PARTICLE_ASTEROID_EXPLOSION,
	PARTICLE_MISSILE_TRAIL,
	PARTICLE_TEXT_POPUP
};

World* world;

const char* ItemNames[ITEM_COUNT] = {
	/* ITEM_MISSILES      */ "Missiles"
};

const char* ItemDescriptions[ITEM_COUNT] = {
	/* ITEM_MISSILES      */ "Launch homing missiles."
};

const char* ActiveItemNames[ACTIVE_ITEM_COUNT] = {
	/* ACTIVE_ITEM_HEAL       */ "Medkit"
};

const char* ActiveItemDescriptions[ACTIVE_ITEM_COUNT] = {
	/* ACTIVE_ITEM_HEAL       */ "Heals 50 HP."
};

static Enemy* make_asteroid(float x, float y, float hsp, float vsp, int type,
							float experience = 0.5f,
							float money = 0.5f) {
	Enemy* e = world->CreateEnemy();
	e->x = x;
	e->y = y;
	e->hsp = hsp;
	e->vsp = vsp;
	e->experience = experience;
	e->money = money;
	e->health = 1.0f;
	e->max_health = 1.0f;
	e->type = type;
	if (e->type == 3) {
		e->radius = ASTEROID_RADIUS_3;
		e->sprite = spr_asteroid3;
	} else if (e->type == 2) {
		e->radius = ASTEROID_RADIUS_2;
		e->sprite = spr_asteroid2;
	} else if (e->type == 1) {
		e->radius = ASTEROID_RADIUS_1;
		e->sprite = spr_asteroid1;
	}
	return e;
}

void World::Init() {
	Player* p = &player;

	p->object_type = ObjType::PLAYER;
	p->id = next_id++;
	p->x = MAP_W / 2.0f;
	p->y = MAP_H / 2.0f;
	// p->active_item = ACTIVE_ITEM_HEAL;
	p->sprite = spr_player_ship;
	// p->items[ITEM_MISSILES]++;

	{
		camera_base_x = p->x;
		camera_base_y = p->y;

		camera_x = camera_base_x;
		camera_y = camera_base_y;

		camera_left = camera_x - camera_w / 2.0f;
		camera_top  = camera_y - camera_h / 2.0f;
	}

	enemies   = (Enemy*)  ecalloc(MAX_ENEMIES,     sizeof *enemies);
	bullets   = (Bullet*) ecalloc(MAX_BULLETS,     sizeof *bullets);
	p_bullets = (Bullet*) ecalloc(MAX_PLR_BULLETS, sizeof *p_bullets);
	allies    = (Ally*)   ecalloc(MAX_ALLIES,      sizeof *allies);
	chests    = (Chest*)  ecalloc(MAX_CHESTS,      sizeof *chests);

	particles.Init();
	particles.SetTypeCircle(PARTICLE_ASTEROID_EXPLOSION,
							4.0f, 4.0f,
							0.0f, 360.0f,
							10.0f, 30.0f,
							{255, 255, 255, 255}, {255, 255, 255, 255},
							2.0f, 2.0f);
	particles.SetTypeCircle(PARTICLE_MISSILE_TRAIL,
							0.0f, 0.0f,
							0.0f, 360.0f,
							20.0f, 20.0f,
							{255, 255, 255, 128}, {128, 128, 128, 128},
							4.0f, 4.0f);
	{
		PartType* t = &particles.types[PARTICLE_TEXT_POPUP];
		t->shape = PartShape::TEXT;
		t->spd_from = t->spd_to = 0.5f;
		t->dir_min = t->dir_max = 90.0f;
		t->lifespan_min = t->lifespan_max = 3.0f * 60.0f;
		t->color_from = t->color_to = {255, 255, 255, 255};
		t->font = fnt_cp437;
		t->xscale_from = t->xscale_to = t->yscale_from = t->yscale_to = 1.5f;
	}

	{
		// spawn chests
		for (int i = 10; i--;) {
			float x = random_range(&rng, 0.0f, MAP_W);
			float y = random_range(&rng, 0.0f, MAP_H);
			float dist = point_distance_wrapped(x, y, p->x, p->y);
			if (dist < 800.0f) {
				i++;
				continue;
			}

			Chest* c = CreateChest();
			c->x = x;
			c->y = y;
			c->sprite = spr_chest;

			if (i == 0) {
				c->type = CHEST_ACTIVE_ITEM;
				c->active_item = ACTIVE_ITEM_HEAL;
				c->cost = 50.0f;
			} else {
				c->type = CHEST_ITEM;
				c->item = ITEM_MISSILES;
				c->cost = 50.0f;
			}
		}
	}

	{
		// Spawn Asteroids.
		int i = 200;
		while (i--) {
			float x = random_range(&rng, 0.0f, MAP_W);
			float y = random_range(&rng, 0.0f, MAP_H);
			float dist = point_distance_wrapped(x, y, p->x, p->y);
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

		if (objects[i].flags & FLAG_INSTANCE_DEAD) continue;

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

static void limit_speed(float* hsp, float* vsp, float max_spd) {
	float spd = length(*hsp, *vsp);
	if (spd > max_spd) {
		*hsp = *hsp / spd * max_spd;
		*vsp = *vsp / spd * max_spd;
	}
}

static void bullet_find_target(Bullet* b,
							   float* out_target_x, float* out_target_y,
							   float* out_target_dist,
							   bool* out_found) {
	*out_found = false;

	float rel_x;
	float rel_y;
	float dist;
	if (Player* p = find_closest(&world->player, 1, b->x, b->y, &rel_x, &rel_y, &dist)) {
		if (dist < DIST_OFFSCREEN) {
			*out_target_x = rel_x;
			*out_target_y = rel_y;
			*out_target_dist = dist;
			*out_found = true;
		}
	}
}

static void p_bullet_find_target(Bullet* b,
								 float* out_target_x, float* out_target_y,
								 float* out_target_dist,
								 bool* out_found) {
	Enemy* enemies = world->enemies;
	int enemy_count = world->enemy_count;

	*out_found = false;

	float target_x;
	float target_y;
	float target_dist;
	bool found = false;

	Enemy* boss = find_closest(enemies,
							   enemy_count,
							   b->x, b->y,
							   &target_x, &target_y,
							   &target_dist,
							   [](Enemy* e) { return e->type >= TYPE_BOSS; });

	if (boss && target_dist < DIST_OFFSCREEN) found = true;

	if (!found) {
		Enemy* enemy = find_closest(enemies,
									enemy_count,
									b->x, b->y,
									&target_x, &target_y,
									&target_dist,
									[](Enemy* e) { return TYPE_ENEMY <= e->type && e->type < TYPE_BOSS; });

		if (enemy && target_dist < DIST_OFFSCREEN) found = true;
	}

	if (!found) {
		Enemy* asteroid = find_closest(enemies,
									   enemy_count,
									   b->x, b->y,
									   &target_x, &target_y,
									   &target_dist,
									   [](Enemy* e) { return e->type < TYPE_BOSS; });

		if (asteroid && target_dist < DIST_OFFSCREEN) found = true;
	}

	if (found) {
		*out_target_x = target_x;
		*out_target_y = target_y;
		*out_target_dist = target_dist;
		*out_found = true;
	}
}

static void update_bullet(Bullet* b,
						  float delta,
						  void (*find_target)(Bullet*, float*, float*, float*, bool*)) {
	switch (b->type) {
		case BulletType::HOMING: {
			float target_x;
			float target_y;
			float target_dist;
			bool found;
			find_target(b, &target_x, &target_y, &target_dist, &found);

			float hmove;
			float vmove;
			if (found && b->lifetime >= 60.0f) {
				float dx = target_x - b->x;
				float dy = target_y - b->y;
				hmove = signf(dx);
				vmove = signf(dy);
			} else {
				hmove =  dcos(b->dir);
				vmove = -dsin(b->dir);
			}

			float acc;
			const float acc_start_t = 30.0f;
			const float acc_grow_t = 120.0f;
			if (b->lifetime >= acc_start_t) {
				acc = lerp(0.0f, b->max_acc,
						   min(b->lifetime - acc_start_t, acc_grow_t) / acc_grow_t);
			} else {
				acc = 0.0f;
			}
			b->hsp += hmove * acc * delta;
			b->vsp += vmove * acc * delta;

			if (b->lifetime >= 60.0f) {
				if (b->hsp != 0.0f || b->vsp != 0.0f) {
					b->dir = point_direction(0.0f, 0.0f, b->hsp, b->vsp);
				}
			}

			limit_speed(&b->hsp, &b->vsp, b->max_spd);

			if (b->lifetime >= 30.0f) {
				b->t += delta;
				const float time = 5.0f;
				if (b->t >= time) {
					world->particles.CreateParticles(b->x, b->y, PARTICLE_MISSILE_TRAIL, 1);
					b->t = fmodf(b->t, time);
				}
			}
			break;
		}
	}

	b->lifetime += delta;
	if (b->lifetime >= b->lifespan) {
		b->flags |= FLAG_INSTANCE_DEAD;
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

		input |= INPUT_FIRE     * key[SDL_SCANCODE_Z];
		input |= INPUT_USE_ITEM * key[SDL_SCANCODE_X];
		input |= INPUT_FOCUS    * key[SDL_SCANCODE_LSHIFT];
		input |= INPUT_BOOST    * key[SDL_SCANCODE_LCTRL];

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

		if (!(p->flags & FLAG_INSTANCE_DEAD)) {
			UpdatePlayer(p, delta);
		}
	}

	for (int i = 0; i < bullet_count; i++) {
		Bullet* b = &bullets[i];

		update_bullet(b, delta, bullet_find_target);

		if (b->flags & FLAG_INSTANCE_DEAD) {
			DestroyBulletByIndex(i);
			i--;
		}
	}

	for (int i = 0; i < p_bullet_count; i++) {
		Bullet* pb = &p_bullets[i];

		update_bullet(pb, delta, p_bullet_find_target);

		if (pb->flags & FLAG_INSTANCE_DEAD) {
			DestroyPlrBulletByIndex(i);
			i--;
		}
	}

	for (int i = 0; i < enemy_count; i++) {
		Enemy* e = &enemies[i];

		if (TYPE_ENEMY <= e->type && e->type < TYPE_BOSS) {
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
					if (!e->not_exact_player_dir || fabsf(angle_difference(e->angle, dir)) > 20.0f) {
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

		e->frame_index = sprite_get_next_frame_index(e->sprite, e->frame_index, delta);
	}

	PhysicsUpdate(delta);

	// :late update

	if (!(player.flags & FLAG_INSTANCE_DEAD)) {
		Player* p = &player;

		p->fire_timer += delta;
		while (p->fire_timer >= 10.0f) {
			if (input & INPUT_FIRE) {
				if (p->fire_queue == 0) p->fire_queue = 4;
			}

			if (p->fire_queue > 0) {
				// player shot type

				auto shoot = [=](float spd, float dir, float dmg, float hoff = 0.0f) {
					Bullet* pb = CreatePlrBullet();
					pb->x = p->x;
					pb->y = p->y;
					pb->dmg = dmg;

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

				auto shoot_homing = [=](float dmg, float hoff) {
					Bullet* pb = CreatePlrBullet();
					pb->x = p->x;
					pb->y = p->y;
					pb->dmg = dmg;
					pb->type = BulletType::HOMING;
					pb->sprite = spr_missile;
					pb->lifespan = 10.0f * 60.0f;
					pb->dir = p->dir;
					pb->max_acc = 0.5f;
					pb->max_spd = 13.5f;

					// side offset
					pb->x += lengthdir_x(hoff, p->dir - 90.0f);
					pb->y += lengthdir_y(hoff, p->dir - 90.0f);

					pb->hsp = p->hsp;
					pb->vsp = p->vsp;

					return pb;
				};

				const float base_dmg = 10.0f;

				switch (p->level) {
					case 0: {
						shoot(20.0f, p->dir, base_dmg);
						break;
					}

					case 1:
					case 2:
					case 3: {
						shoot(20.0f, p->dir - 4.0f, base_dmg * 0.75f);
						shoot(20.0f, p->dir + 4.0f, base_dmg * 0.75f);
						break;
					}

					default:
					case 4: {
						shoot(20.0f, p->dir, base_dmg * 0.75f,  10.0f);
						shoot(20.0f, p->dir, base_dmg * 0.75f, -10.0f);

						shoot(20.0f, p->dir - 30.0f, base_dmg * 0.50f,  10.0f);
						shoot(20.0f, p->dir + 30.0f, base_dmg * 0.50f, -10.0f);
						break;
					}
				}

				if (p->items[ITEM_MISSILES] > 0) {
					if (p->shot % 15 == 0) {
						shoot_homing(base_dmg,  25.0f);
						shoot_homing(base_dmg, -25.0f);
					}
				}

				play_sound(snd_shoot, p->x, p->y, 10);

				p->fire_queue--;
				p->shot++;
			}

			p->fire_timer -= 10.0f;
		}

		p->invincibility -= delta;
		if (p->invincibility < 0.0f) p->invincibility = 0.0f;
	}

	{
		Player* p = &player;

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
				camera_x += random_range(&rng_visual, -range, range);
				camera_y += random_range(&rng_visual, -range, range);
			}
			// else if (screenshake_time <= 6.0f) {  // short
			// 	camera_x += float((frame % 2) * 2 - 1) * screenshake_intensity;
			// }
			else {
				float range = screenshake_intensity;
				camera_x += random_range(&rng_visual, -range, range);
				camera_y += random_range(&rng_visual, -range, range);
			}
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

	// if (game->key_pressed[SDL_SCANCODE_TAB]) {
	// 	hide_interface ^= true;
	// }
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

static void use_active_item(Player* p) {
	if (p->active_item != ACTIVE_ITEM_NONE && p->active_item_cooldown == 0.0f) {
		switch (p->active_item) {
			case ACTIVE_ITEM_HEAL: {
				p->health += 50.0f;
				break;
			}
		}

		p->active_item_cooldown = ACTIVE_ITEM_COOLDOWN;
	}
}

static float get_next_level_exp(Player* p) {
	return 100.0f + 10.0f * float(p->level);
}

void World::UpdatePlayer(Player* p, float delta) {
	if (p->flags & FLAG_INSTANCE_DEAD) return;

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
									[](Enemy* e) { return e->type >= TYPE_ENEMY; })) {
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
	limit_speed(&p->hsp, &p->vsp, max_spd);

	// use active item
	if (input_press & INPUT_USE_ITEM) {
		use_active_item(p);
	}

	// heal
	p->health += (1.0f / 120.0f) * delta;
	p->health = min(p->health, p->max_health);

	// boost
	if (!(input & INPUT_BOOST)) {
		p->boost += (1.0f / 20.0f) * delta;
		p->boost = min(p->boost, p->max_boost);
	}

	// level up
	{
		float next_level_exp = get_next_level_exp(p);
		if (p->experience >= next_level_exp) {
			p->level++;
			p->experience -= next_level_exp;
			play_sound(snd_powerup, p->x, p->y);
		}
	}

	p->active_item_cooldown = max(p->active_item_cooldown - delta, 0.0f);

	const u8* key = SDL_GetKeyboardState(nullptr);

	// open chests
	for (int i = 0; i < chest_count; i++) {
		Chest* c = &chests[i];

		if (c->opened) continue;

		if (circle_vs_circle_wrapped(p->x, p->y, p->radius, c->x, c->y, c->radius)) {
			if (input_press & INPUT_FIRE) {
				if (p->money >= c->cost || key[SDL_SCANCODE_LALT]) {
					// get item
					switch (c->type) {
						case CHEST_ITEM: {
							p->items[c->item]++;
							Particle* p = particles.CreateParticles(c->x, c->y, PARTICLE_TEXT_POPUP, 1);
							p->text = ItemNames[c->item];
							break;
						}
						case CHEST_ACTIVE_ITEM: {
							p->active_item = c->active_item;
							Particle* p = particles.CreateParticles(c->x, c->y, PARTICLE_TEXT_POPUP, 1);
							p->text = ActiveItemNames[c->active_item];
							break;
						}
					}
					p->money -= c->cost;
					c->opened = true;
				}
			}
		}
	}
}

void World::PhysicsUpdate(float delta) {
	if (!(player.flags & FLAG_INSTANCE_DEAD)) {
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

	if (!(player.flags & FLAG_INSTANCE_DEAD)) {
		Player* p = &player;

		if (p->invincibility == 0.0f) {
			for (int i = 0; i < bullet_count; i++) {
				Bullet* b = &bullets[i];
				if (circle_vs_circle_wrapped(p->x, p->y, p->radius, b->x, b->y, b->radius)) {
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

				if (circle_vs_circle_wrapped(p->x, p->y, p->radius, e->x, e->y, e->radius)) {
					float contact_damage = 15.0f;
					if (e->type < TYPE_ENEMY) contact_damage = 10.0f;

					player_get_hit(p, contact_damage);

					float split_dir = point_direction_wrapped(p->x, p->y, e->x, e->y);
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

		auto collide_with_bullets = [=](Bullet* bullets, int bullet_count,
										void (World::*destroy)(int),
										bool player = false) {
			for (int bullet_idx = 0; bullet_idx < bullet_count;) {
				Bullet* b = &bullets[bullet_idx];

				if (circle_vs_circle_wrapped(e->x, e->y, e->radius, b->x, b->y, b->radius)) {
					float split_dir = point_direction_wrapped(b->x, b->y, e->x, e->y);

					float dmg = b->dmg;
					(this->*destroy)(bullet_idx);
					bullet_count--;

					if (!enemy_get_hit(e, dmg, split_dir)) {
						if (player) {
							p->experience += e->experience;
							p->money += e->money;

							// todo
							// player can get power while dead
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

		// if (e->type < TYPE_ENEMY) {
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

	if (p->health <= 0.0f) {
		p->flags |= FLAG_INSTANCE_DEAD;
		p->hsp = 0.0f;
		p->vsp = 0.0f;
	}

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
		switch (e->type) {
			case 2: {
				make_asteroid(e->x, e->y, e->hsp + lengthdir_x(1.0f, split_dir + 90.0f), e->vsp + lengthdir_y(1.0f, split_dir + 90.0f), 1, e->experience / 2.0f, 0.0f);
				make_asteroid(e->x, e->y, e->hsp + lengthdir_x(1.0f, split_dir - 90.0f), e->vsp + lengthdir_y(1.0f, split_dir - 90.0f), 1, e->experience / 2.0f, 0.0f);
				break;
			}
			case 3: {
				make_asteroid(e->x, e->y, e->hsp + lengthdir_x(1.0f, split_dir + 90.0f), e->vsp + lengthdir_y(1.0f, split_dir + 90.0f), 2, e->experience / 2.0f, 0.0f);
				make_asteroid(e->x, e->y, e->hsp + lengthdir_x(1.0f, split_dir - 90.0f), e->vsp + lengthdir_y(1.0f, split_dir - 90.0f), 2, e->experience / 2.0f, 0.0f);
				break;
			}
		}

		Mix_Chunk* sound = (e->type >= TYPE_BOSS) ? snd_boss_explode : snd_explode;
		play_sound(sound, e->x, e->y);

		if (e->type >= TYPE_BOSS) {
			ScreenShake(e->x, e->y, 15.0f, 60.0f, true);
			game->sleep(e->x, e->y, 100);
		} else if (e->type >= TYPE_ENEMY) {
			ScreenShake(e->x, e->y, 5.0f, 10.0f);
			game->sleep(e->x, e->y, 50);
		} else {
			// ScreenShake(e->x, e->y, 2.0f, 8.0f);
			// game->sleep(e->x, e->y, 20);
		}

		particles.CreateParticles(e->x, e->y, PARTICLE_ASTEROID_EXPLOSION, 4);

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

static void draw_object(Object* inst,
						float angle = 0.0f,
						float xscale = 1.0f, float yscale = 1.0f,
						SDL_Color color = {255, 255, 255, 255}) {
	DrawSpriteCamWarped(inst->sprite, int(inst->frame_index),
						inst->x, inst->y,
						angle,
						xscale, yscale,
						color);
}

static void draw_bullet(Bullet* b, bool player) {
	switch (b->type) {
		case BulletType::NORMAL: {
			if (player) {
				DrawCircleCamWarped(b->x, b->y, b->radius);
			} else {
				DrawCircleCamWarped(b->x, b->y, b->radius + 1.0f, {255, 0, 0, 255});
				DrawCircleCamWarped(b->x, b->y, b->radius - 1.0f);
			}
			break;
		}

		case BulletType::HOMING: {
			SDL_Color col = {255, 255, 255, 255};
			if (!player) col = {255, 0, 0, 255};
			draw_object(b, b->dir, 1.0f, 1.0f, col);
			break;
		}
	}
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

	// draw chests
	for (int i = 0; i < chest_count; i++) {
		Chest* c = &chests[i];

		SDL_Color col = {255, 255, 255, 255};
		if (c->opened) col.a = 128;

		c->frame_index = float(2 * c->type + c->opened);
		draw_object(c, 0.0f, 1.0f, 1.0f, col);

		if (c->opened) continue;

		Player* p = &player;
		if (circle_vs_circle_wrapped(p->x, p->y, p->radius, c->x, c->y, c->radius)) {
			char buf[32];
			stb_snprintf(buf, sizeof buf, "[Z] Open Chest - $%.0f", c->cost);
			DrawTextShadowCamWarped(renderer, fnt_cp437, buf,
									int(c->x),
									int(c->y) + 50,
									HALIGN_CENTER, VALIGN_MIDDLE);
		}
	}

	// Draw enemies.
	for (int i = 0; i < enemy_count; i++) {
		Enemy* e = &enemies[i];
		if (show_hitboxes) DrawCircleCamWarped(e->x, e->y, e->radius, {255, 0, 0, 255});
		draw_object(e, e->angle);
	}

	{
		// Draw player.

		Player* p = &player;

		SDL_Color col = {255, 255, 255, 255};
		if (p->invincibility > 0.0f) {
			if ((SDL_GetTicks() / 16 / 4) % 2) col.a = 64;
			else col.a = 192;
		}

		if (show_hitboxes) DrawCircleCamWarped(p->x, p->y, p->radius, {128, 255, 128, 255});
		draw_object(p, p->dir, 1.0f, 1.0f, col);
	}

	// draw allies
	for (int i = 0; i < ally_count; i++) {
		Ally* a = &allies[i];
		draw_object(a);
	}

	// Draw bullets.
	for (int i = 0; i < bullet_count; i++) {
		Bullet* b = &bullets[i];
		draw_bullet(b, false);
	}

	// Draw player bullets.
	for (int i = 0; i < p_bullet_count; i++) {
		Bullet* pb = &p_bullets[i];
		draw_bullet(pb, true);
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

	Player* p = &player;

	// top left
	int x = 10;
	int y = 10;

	{
		// Draw map.

		// SDL_Rect dest = {0, y, INTERFACE_MAP_W, INTERFACE_MAP_H};
		// SDL_RenderCopy(renderer, interface_map_texture, nullptr, &dest);

		int w = 100;
		int h = 100;
		SDL_Rect src = {
			(int) (p->x / MAP_W * (float)INTERFACE_MAP_W) - w / 2,
			(int) (p->y / MAP_H * (float)INTERFACE_MAP_H) - h / 2,
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
		draw_bar(x, y, w, h, p->health / p->max_health, {255, 0, 0, 255});

		char buf[32];
		stb_snprintf(buf, sizeof(buf), "HEALTH %.0f/%.0f", p->health, p->max_health);
		DrawText(renderer, fnt_cp437, buf, x + w / 2, y + h / 2, HALIGN_CENTER, VALIGN_MIDDLE);

		y += h;
		y += 4;
	}
	{
		// Draw boost meter.
		int w = 180;
		int h = 18;
		draw_bar(x, y, w, h, p->boost / p->max_boost, {0, 0, 255, 255});

		char buf[32];
		stb_snprintf(buf, sizeof(buf), "BOOST %.0f/%.0f", p->boost, p->max_boost);
		DrawText(renderer, fnt_cp437, buf, x + w / 2, y + h / 2, HALIGN_CENTER, VALIGN_MIDDLE);

		y += h;
		y += 4;
	}
	{
		// draw exp meter
		int w = 180;
		int h = 18;
		draw_bar(x, y, w, h, p->experience / get_next_level_exp(p), {128, 128, 255, 255});

		char buf[32];
		stb_snprintf(buf, sizeof(buf), "LEVEL %d (%.0f/%.0f)", p->level + 1, p->experience, get_next_level_exp(p));
		DrawText(renderer, fnt_cp437, buf, x + w / 2, y + h / 2, HALIGN_CENTER, VALIGN_MIDDLE);

		y += h;
		y += 4;
	}
	y += 4;
	{
		// text info
		y -= 2;
		char buf[100];
		stb_snprintf(buf, sizeof(buf),
					 "X: %f\n"
					 "Y: %f\n"
					 "MONEY: %.2f\n",
					 interface_x,
					 interface_y,
					 p->money);
		y = DrawText(renderer, fnt_cp437, buf, x, y).y;
	}

	// top right
	x = game->ui_w - 10;
	y = 10;

	SDL_Rect active_item_rect;
	{
		// draw active item
		float xx = float(x - spr_active_item->width);
		float yy = float(y);
		SDL_Color color = {255, 255, 255, 255};
		if (p->active_item_cooldown > 0.0f) {
			color = {128, 128, 128, 255};
		}
		u8 frame_index = p->active_item;
		frame_index++;
		DrawSprite(spr_active_item, frame_index, xx, yy, 0.0f, 1.0f, 1.0f, color);

		SDL_Rect rect = {int(xx), int(yy), spr_active_item->width, spr_active_item->height};
		active_item_rect = rect;

		if (p->active_item_cooldown > 0.0f) {
			char buf[16];
			stb_snprintf(buf, sizeof(buf), "%.0f", ceilf(p->active_item_cooldown / 60.0f));
			DrawText(renderer,
					 fnt_cp437,
					 buf,
					 rect.x + rect.w / 2,
					 rect.y + rect.h / 2,
					 HALIGN_CENTER,
					 VALIGN_MIDDLE,
					 {255, 255, 255, 255},
					 1.5f, 1.5f);
		}

		// outline
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
		SDL_RenderDrawRect(renderer, &rect);
	}

	const u8* key = SDL_GetKeyboardState(nullptr);
	if (key[SDL_SCANCODE_TAB]) {
		int scale = 2;

		int w = INTERFACE_MAP_W * scale;
		int h = INTERFACE_MAP_H * scale;

		int x = (game->ui_w - w) / 2;
		int y = (game->ui_h - h) / 2;

		SDL_Rect dest = {x, y, w, h};
		SDL_RenderCopy(renderer, interface_map_texture, nullptr, &dest);

		// outline
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
		SDL_RenderDrawRect(renderer, &dest);
	}

	// Draw boss healthbar.
	for (int i = 0; i < enemy_count; i++) {
		Enemy* e = &enemies[i];
		
		if (e->type < TYPE_BOSS) continue;

		int w = 450;
		int h = 18;
		SDL_Rect back = {(game->ui_w - w) / 2, 20, w, h};
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderFillRect(renderer, &back);
		int filled_w = (int) (e->health / e->max_health * (float)w);
		SDL_Rect filled = {back.x, back.y, filled_w, back.h};
		SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
		SDL_RenderFillRect(renderer, &filled);

		// Outline.
		SDL_RenderDrawRect(renderer, &back);

		char buf[32];
		stb_snprintf(buf, sizeof(buf), "BOSS %.0f/%.0f", e->health, e->max_health);
		DrawText(renderer, fnt_cp437, buf, back.x + w / 2, back.y + h / 2, HALIGN_CENTER, VALIGN_MIDDLE);

		break;
	}

	// middle bottom

	// draw items
	x = 10;
	y = game->ui_h - spr_item->height - 10;
	for (u8 i = 0; i < ITEM_COUNT; i++) {
		if (p->items[i] > 0) {
			DrawSprite(spr_item, i, float(x), float(y), 0.0f, 1.0f, 1.0f, {255, 255, 255, 128});
			x += spr_item->width + 10;
		}
	}

	// mouse hover text info

	SDL_Point mouse = {int(game->ui_mouse_x), int(game->ui_mouse_y)};

	// active item
	if (SDL_PointInRect(&mouse, &active_item_rect)) {
		char buf[256];
		if (p->active_item == ACTIVE_ITEM_NONE) {
			stb_snprintf(buf, sizeof buf, "\nActive item\n\nActive item desc.");
		} else {
			stb_snprintf(buf, sizeof buf,
							"\n%s\n\n%s",
							ActiveItemNames[p->active_item],
							ActiveItemDescriptions[p->active_item]);
		}
		int x = mouse.x;
		int y = mouse.y;
		Font* font = fnt_cp437;
		SDL_Point size = MeasureText(font, buf);
		// x = min(x, game->ui_w - 10 - size.x);
		// y = min(y, game->ui_h - 10 - size.y);
		if (x >= game->ui_w - 10 - size.x) x -= size.x;
		if (y >= game->ui_h - 10 - size.y) y -= size.y;
		DrawTextShadow(renderer,
						font,
						buf,
						x, y);
	}

	// items
	x = 10;
	y = game->ui_h - spr_item->height - 10;
	for (u8 i = 0; i < ITEM_COUNT; i++) {
		if (p->items[i] > 0) {
			SDL_Rect rect = {x, y, spr_item->width, spr_item->height};
			if (SDL_PointInRect(&mouse, &rect)) {
				char buf[256];
				stb_snprintf(buf, sizeof buf,
							 "\n%s\n\n%s",
							 ItemNames[i],
							 ItemDescriptions[i]);
				int x = mouse.x;
				int y = mouse.y;
				Font* font = fnt_cp437;
				SDL_Point size = MeasureText(font, buf);
				if (x >= game->ui_w - 10 - size.x) x -= size.x;
				if (y >= game->ui_h - 10 - size.y) y -= size.y;
				DrawTextShadow(renderer,
							   font,
							   buf,
							   x, y);
			}

			x += spr_item->width + 10;
		}
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
				if (enemies[i].type < TYPE_ENEMY) {
					SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
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
					SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
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

SDL_Point DrawTextCamWarped(SDL_Renderer* renderer, Font* font, const char* text,
							int x, int y,
							int halign, int valign,
							SDL_Color color,
							float xscale, float yscale) {
	SDL_Point result = {};

	auto draw = [renderer, font, text, x, y, halign, valign, color, xscale, yscale, &result](int xoff, int yoff) {
		SDL_Point size = MeasureText(font, text, xscale, yscale);

		SDL_Rect contains = {
			x + xoff,
			y + yoff,
			size.x,
			size.y
		};

		SDL_Rect screen = {
			int(world->camera_left),
			int(world->camera_top),
			int(world->camera_w),
			int(world->camera_h)
		};

		if (SDL_HasIntersection(&contains, &screen)) {
			result = DrawText(renderer,
							  font,
							  text,
							  contains.x - screen.x,
							  contains.y - screen.y,
							  halign, valign,
							  color, xscale, yscale);
		}
	};

	draw((int) -MAP_W, (int) -MAP_H);
	draw((int)  0,     (int) -MAP_H);
	draw((int)  MAP_W, (int) -MAP_H);

	draw((int) -MAP_W, (int) 0);
	draw((int)  0,     (int) 0);
	draw((int)  MAP_W, (int) 0);

	draw((int) -MAP_W, (int) MAP_H);
	draw((int)  0,     (int) MAP_H);
	draw((int)  MAP_W, (int) MAP_H);

	return result;
}

SDL_Point DrawTextShadowCamWarped(SDL_Renderer* renderer, Font* font, const char* text,
								  int x, int y,
								  int halign, int valign,
								  SDL_Color color,
								  float xscale, float yscale) {
	DrawTextCamWarped(renderer, font, text, x + 1, y + 1, halign, valign, {0, 0, 0, 255}, xscale, yscale);
	return DrawTextCamWarped(renderer, font, text, x, y, halign, valign, color, xscale, yscale);
}

static void CleanupObject(Enemy* e) {
	if (e->co) {
		mco_destroy(e->co);
	}
}
static void CleanupObject(Bullet* b) {}
static void CleanupObject(Ally* a) {}
static void CleanupObject(Chest* c) {}

template <typename T>
static void DestroyObjectByIndex(T* &objects, int &object_count,
								 int object_idx) {
	if (object_count == 0) {
		return;
	}

	CleanupObject(&objects[object_idx]);

	for (int i = object_idx + 1; i < object_count; i++) {
		objects[i - 1] = objects[i];
	}
	object_count--;
}

template <typename T>
static T* CreateObject(T* &objects, int &object_count,
					   int max_objects, ObjType object_type, instance_id &next_id,
					   const char* msg) {
	if (object_count == max_objects) {
		SDL_Log(msg);
		DestroyObjectByIndex<T>(objects, object_count, 0);
	}

	T* result = &objects[object_count];
	*result = {};
	result->object_type = object_type;
	result->id = next_id++;
	object_count++;

	return result;
}

Enemy*  World::CreateEnemy    () { return CreateObject(enemies,   enemy_count,    MAX_ENEMIES,     ObjType::ENEMY,         next_id, "Enemy limit hit."); }
Bullet* World::CreateBullet   () { return CreateObject(bullets,   bullet_count,   MAX_BULLETS,     ObjType::BULLET,        next_id, "Bullet limit hit."); }
Bullet* World::CreatePlrBullet() { return CreateObject(p_bullets, p_bullet_count, MAX_PLR_BULLETS, ObjType::PLAYER_BULLET, next_id, "Player bullets limit hit."); }
Ally*   World::CreateAlly     () { return CreateObject(allies,    ally_count,     MAX_ALLIES,      ObjType::ALLY,          next_id, "Allies limit hit."); }
Chest*  World::CreateChest    () { return CreateObject(chests,    chest_count,    MAX_CHESTS,      ObjType::CHEST,         next_id, "Chests limit hit."); }

void World::DestroyEnemyByIndex    (int enemy_idx)    { DestroyObjectByIndex(enemies,   enemy_count,    enemy_idx); }
void World::DestroyBulletByIndex   (int bullet_idx)   { DestroyObjectByIndex(bullets,   bullet_count,   bullet_idx); }
void World::DestroyPlrBulletByIndex(int p_bullet_idx) { DestroyObjectByIndex(p_bullets, p_bullet_count, p_bullet_idx); }
void World::DestroyAllyByIndex     (int ally_idx)     { DestroyObjectByIndex(allies,    ally_count,     ally_idx); }
void World::DestroyChestByIndex    (int chest_idx)    { DestroyObjectByIndex(chests,    chest_count,    chest_idx); }
