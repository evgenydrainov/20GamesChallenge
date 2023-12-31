static void wait(mco_coro* co, int t) {
	while (t--) {
		mco_yield(co);
	}
}

static Bullet* shoot(Enemy* e, float spd, float dir) {
	Bullet* b = world->CreateBullet();
	b->x = e->x;
	b->y = e->y;
	b->hsp = e->hsp + lengthdir_x(spd, dir);
	b->vsp = e->vsp + lengthdir_y(spd, dir);

	play_sound(game->snd_shoot, e->x, e->y);

	return b;
}

static Bullet* _shoot(Enemy* e, float spd, float dir) {
	Bullet* b = world->CreateBullet();
	b->x = e->x;
	b->y = e->y;
	b->hsp = lengthdir_x(spd, dir);
	b->vsp = lengthdir_y(spd, dir);

	return b;
}

#define self ((Enemy*)(co->user_data))

template <typename F>
static void shoot_radial(mco_coro* co, int n, float dir_diff, const F& f) {
	for (int i = 0; i < n; i++) {
		float a = -((float)n - 1.0f) / 2.0f + (float)i;
		Bullet* b = f(co, i);

		float spd = length(b->hsp, b->vsp);
		float dir = point_direction(0.0f, 0.0f, b->hsp, b->vsp);
		dir += a * dir_diff;
		b->hsp = lengthdir_x(spd, dir);
		b->vsp = lengthdir_y(spd, dir);

		b->hsp += self->hsp;
		b->vsp += self->vsp;
	}

	play_sound(game->snd_shoot, self->x, self->y);
}

#include "enemy_scripts.h"
#include "boss_scripts.h"

#undef self

static void spawn_enemies(mco_coro* co) {
	auto spawn_ships = [](mco_coro* co, int i) {
		float dir = world->random.range(0.0f, 360.0f);
		float x = world->player.x - lengthdir_x(4000.0f, dir);
		float y = world->player.y - lengthdir_y(4000.0f, dir);

		while (i--) {
			Enemy* e = world->CreateEnemy();
			
			e->x = x;
			e->y = y;
			e->max_spd = world->random.range(10.0f, 11.0f);
			e->hsp = lengthdir_x(e->max_spd, world->player.dir);
			e->vsp = lengthdir_y(e->max_spd, world->player.dir);
			e->angle = world->player.dir;

			e->type = TYPE_ENEMY;
			e->sprite = &game->spr_player_ship;
			mco_desc desc = mco_desc_init(enemy_ship, 0);
			mco_create(&e->co, &desc);
			e->acc = world->random.range(0.25f, 0.35f);
			e->stop_when_close_to_player = world->random.range(0.0f, 1.0f) > 0.5f;
			e->not_exact_player_dir = world->random.range(0.0f, 1.0f) > 0.5f;

			x += world->random.range(-50.0f, 50.0f);
			y += world->random.range(-50.0f, 50.0f);

			wait(co, 30);
		}
	};

	auto enemy_count = []() {
		int result = 0;
		for (int i = 0; i < world->enemy_count; i++) {
			if (world->enemies[i].type >= TYPE_ENEMY) {
				result++;
			}
		}
		return result;
	};

	// world->player.power = 50;
	// goto l_boss;

	wait(co, 40 * 60);

	for (int i = 5; i--;) {
		spawn_ships(co, 1);
		wait(co, 15 * 60);
		while (enemy_count() > 0) wait(co, 1);
	}

	for (int i = 3; i--;) {
		spawn_ships(co, 3);
		wait(co, 20 * 60);
		while (enemy_count() > 0) wait(co, 1);
	}

	for (int i = 2; i--;) {
		spawn_ships(co, 5);
		wait(co, 20 * 60);
		while (enemy_count() > 0) wait(co, 1);
	}

	l_boss:
	{
		float dir = world->random.range(0.0f, 360.0f);
		float x = world->player.x - lengthdir_x(1500.0f, dir);
		float y = world->player.y - lengthdir_y(1500.0f, dir);

		Enemy* e = world->CreateEnemy();
		e->x = x;
		e->y = y;
		e->radius = 25.0f;
		e->type = TYPE_BOSS;
		e->health = 2000.0f;
		e->max_health = 2000.0f;
		e->sprite = &game->spr_invader;
		mco_desc desc = mco_desc_init(invader_script2, 0);
		mco_create(&e->co, &desc);
	}
}
