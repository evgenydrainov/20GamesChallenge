#include "scripts_common.h"

void stage0_script(mco_coro* co) {
	// int a;
	// SDL_Log("%lld", co->stack_size - (i64(&a) - i64(co->stack_base)));

	extern void enemy0_script(mco_coro*);
	extern void boss0_script(mco_coro*);

	auto spawn_ships = [=](int i) {
		float dir = random_range(&world->rng, 0.0f, 360.0f);
		float x = world->player.x - lengthdir_x(4000.0f, dir);
		float y = world->player.y - lengthdir_y(4000.0f, dir);

		while (i--) {
			Enemy* e = world->CreateEnemy();

			e->x = x;
			e->y = y;
			e->max_spd = random_range(&world->rng, 10.0f, 11.0f);
			e->hsp = lengthdir_x(e->max_spd, world->player.dir);
			e->vsp = lengthdir_y(e->max_spd, world->player.dir);
			e->angle = world->player.dir;

			e->enemy_type = TYPE_ENEMY;
			e->sprite = spr_player_ship;
			mco_desc desc = mco_desc_init(enemy0_script, 0);
			mco_create(&e->co, &desc);
			e->acc = random_range(&world->rng, 0.25f, 0.35f);
			e->stop_when_close_to_player = random_range(&world->rng, 0.0f, 1.0f) > 0.5f;
			e->not_exact_player_dir = random_range(&world->rng, 0.0f, 1.0f) > 0.5f;

			x += random_range(&world->rng, -50.0f, 50.0f);
			y += random_range(&world->rng, -50.0f, 50.0f);

			wait(co, 30);
		}
	};

	auto enemy_count = []() {
		int result = 0;
		for (int i = 0; i < world->enemy_count; i++) {
			if (world->enemies[i].enemy_type >= TYPE_ENEMY) {
				result++;
			}
		}
		return result;
	};

	// world->player.power = 50;
	// goto l_boss;

	wait(co, 40 * 60);

	for (int i = 5; i--;) {
		spawn_ships(1);
		wait(co, 10 * 60);
		while (enemy_count() > 0) wait(co, 1);
		wait(co, 5 * 60);
	}

	for (int i = 3; i--;) {
		spawn_ships(3);
		wait(co, 15 * 60);
		while (enemy_count() > 0) wait(co, 1);
		wait(co, 5 * 60);
	}

	for (int i = 2; i--;) {
		spawn_ships(5);
		wait(co, 15 * 60);
		while (enemy_count() > 0) wait(co, 1);
		wait(co, 5 * 60);
	}

l_boss:
	{
		float dir = random_range(&world->rng, 0.0f, 360.0f);
		float x = world->player.x - lengthdir_x(1500.0f, dir);
		float y = world->player.y - lengthdir_y(1500.0f, dir);

		Enemy* e = world->CreateEnemy();
		e->x = x;
		e->y = y;
		e->radius = 25.0f;
		e->enemy_type = TYPE_BOSS;
		e->health = 2000.0f;
		e->max_health = 2000.0f;
		e->sprite = spr_invader;
		mco_desc desc = mco_desc_init(boss0_script, 0);
		mco_create(&e->co, &desc);
	}
}
