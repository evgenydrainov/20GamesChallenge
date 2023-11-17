#include "scripts_common.h"

static Enemy* _create_enemy(float x, float y, float dir,
							int type,
							float max_spd,
							float acc,
							Sprite* sprite,
							mco_func* func) {
	Enemy* e = world->CreateEnemy();

	e->x = x;
	e->y = y;
	e->max_spd = max_spd;
	e->hsp = lengthdir_x(e->max_spd, dir);
	e->vsp = lengthdir_y(e->max_spd, dir);
	e->angle = dir;

	e->type = type;
	e->sprite = sprite;
	mco_desc desc = mco_desc_init(func, 0);
	mco_create(&e->co, &desc);
	e->acc = acc;

	e->experience = 2.5f;
	e->money = 5.0f;

	return e;
}

static Enemy* create_enemy(float x, float y, float dir) {
	extern mco_func script_enemy;

	float max_spd = random_range(&world->rng, 10.0f, 11.0f);
	float acc = random_range(&world->rng, 0.25f, 0.35f);

	Enemy* e = _create_enemy(x, y, dir,
							 TYPE_ENEMY,
							 max_spd, acc,
							 spr_player_ship,
							 script_enemy);

	e->stop_when_close_to_player = random_chance(&world->rng, 50.0f);
	e->not_exact_player_dir = random_chance(&world->rng, 50.0f);

	return e;
}

static Enemy* create_enemy_spread(float x, float y, float dir) {
	extern mco_func script_enemy_spread;

	float max_spd = 10.0f;
	float acc = 0.3f;

	Enemy* e = _create_enemy(x, y, dir,
							 TYPE_ENEMY_SPREAD,
							 max_spd, acc,
							 spr_player_ship,
							 script_enemy_spread);

	return e;
}

static Enemy* create_enemy_missile(float x, float y, float dir) {
	extern mco_func script_enemy_missile;

	float max_spd = 8.0f;
	float acc = 0.2f;

	Enemy* e = _create_enemy(x, y, dir,
							 TYPE_ENEMY_MISSILE,
							 max_spd, acc,
							 spr_player_ship,
							 script_enemy_missile);

	return e;
}

static Enemy* create_boss(float x, float y) {
	extern mco_func boss0_script;

	Enemy* e = world->CreateEnemy();
	e->x = x;
	e->y = y;
	e->radius = 25.0f;
	e->type = TYPE_BOSS;
	e->health = 2000.0f;
	e->max_health = 2000.0f;
	e->sprite = spr_invader;
	mco_desc desc = mco_desc_init(boss0_script, 0);
	mco_create(&e->co, &desc);

	return e;
}

/*
static void spawn_ships(mco_coro* co, int i) {
	float dir = random_range(&world->rng, 0.0f, 360.0f);
	float x = world->player.x - lengthdir_x(4000.0f, dir);
	float y = world->player.y - lengthdir_y(4000.0f, dir);

	while (i--) {
		create_enemy(x, y, dir + 180.0f);

		x += random_range(&world->rng, -50.0f, 50.0f);
		y += random_range(&world->rng, -50.0f, 50.0f);

		wait(co, 30);
	}
}

void stage0_script(mco_coro* co) {
	// int a;
	// SDL_Log("%lld", co->stack_size - (i64(&a) - i64(co->stack_base)));

	// world->player.power = 200;
	// goto l_boss;

	wait(co, 40 * 60);

	for (int i = 5; i--;) {
		spawn_ships(co, 1);
		wait(co, 10 * 60);
		while (world->get_enemy_count() > 0) wait(co, 1);
		wait(co, 5 * 60);
	}

	for (int i = 3; i--;) {
		spawn_ships(co, 3);
		wait(co, 15 * 60);
		while (world->get_enemy_count() > 0) wait(co, 1);
		wait(co, 5 * 60);
	}

	for (int i = 2; i--;) {
		spawn_ships(co, 5);
		wait(co, 15 * 60);
		while (world->get_enemy_count() > 0) wait(co, 1);
		wait(co, 5 * 60);
	}

l_boss:
	{
		float dir = random_range(&world->rng, 0.0f, 360.0f);
		float x = world->player.x - lengthdir_x(1500.0f, dir);
		float y = world->player.y - lengthdir_y(1500.0f, dir);

		create_boss(x, y);
	}
}
//*/

/*
template <typename F>
static bool ai_step(int cost, float chance,
					int &points, int &total_points,
					const F& f) {
	if (points >= cost) {
		if (random_chance(&world->rng, chance)) {
			float dir = random_range(&world->rng, 0.0f, 360.0f);
			float x = world->player.x - lengthdir_x(4000.0f, dir);
			float y = world->player.y - lengthdir_y(4000.0f, dir);
			f(x, y, dir);

			points -= cost;
			total_points += cost;

			return true;
		}
	}

	return false;
}

static void ai_tick(mco_coro* co, int &points, int &total_points) {
	int count = 0;

	for (;;) {
		bool spawned = false;

		if (ai_step(30, 25.0f, points, total_points, create_enemy_spread)) {spawned = true; count++;}

		if (ai_step(20, 25.0f, points, total_points, create_enemy_missile)) {spawned = true; count++;}

		if (ai_step(10, 50.0f, points, total_points, create_enemy)) {spawned = true; count++;}

		if (!spawned || count >= 5) {
			break;
		}
	}
}

void stage0_script(mco_coro* co) {
	int points = 0;
	int total_points = 0;

	int tick_wait_time = random_range(&world->rng, 15, 30);
	int tick_wait_timer = 0;

	world->ai_points = &points;
	world->ai_tick_wait_time = &tick_wait_time;
	world->ai_tick_wait_timer = &tick_wait_timer;

	for (;;) {
		if (world->get_enemy_count() < 10) {
			points++;
		}

		tick_wait_timer++;
		if (tick_wait_timer >= tick_wait_time) {
			ai_tick(co, points, total_points);

			tick_wait_time = random_range(&world->rng, 15, 30);
			tick_wait_timer = 0;
		}

		wait(co, 60);
	}
}
//*/

//*
static void spawn_wave(mco_coro* co, int wave) {
	int count = wave / 5 + 1;

	float missile_chance = float(wave / 5) / 4.0f * 25.0f;
	float spread_chance  = float(wave / 5) / 4.0f * 25.0f;

	while (count--) {
		float dir = random_range(&world->rng, 0.0f, 360.0f);
		float x = world->player.x - lengthdir_x(4000.0f, dir);
		float y = world->player.y - lengthdir_y(4000.0f, dir);

		if (random_chance(&world->rng, missile_chance)) {
			create_enemy_missile(x, y, dir);
		} else if (random_chance(&world->rng, spread_chance)) {
			create_enemy_spread(x, y, dir);
		} else {
			create_enemy(x, y, dir);
		}

		wait(co, 60);
	}
}

void stage0_script(mco_coro* co) {
	wait(co, 60 * 60);

	int wave = 1;

	for (int i = 20; i--;) {
		spawn_wave(co, wave++);

		wait(co, 10 * 60);

		while (world->get_enemy_count() > 1) {
			wait(co, 1);
		}

		wait(co, 10 * 60);
	}
}
//*/
