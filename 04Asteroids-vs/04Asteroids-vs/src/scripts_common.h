#pragma once

#include "Game.h"
#include "Assets.h"
#include "Audio.h"
#include "mathh.h"

typedef void mco_func(mco_coro*);

static void wait(mco_coro* co, int t) {
	while (t--) {
		mco_yield(co);
	}
}

static Bullet* shoot(Enemy* e, float spd, float dir,
					 bool _play_sound = true, bool add_enemy_speed = true) {
	Bullet* b = world->CreateBullet();
	b->x = e->x;
	b->y = e->y;
	b->hsp = lengthdir_x(spd, dir);
	b->vsp = lengthdir_y(spd, dir);
	b->dmg = 15.0f;

	if (add_enemy_speed) {
		b->hsp += e->hsp;
		b->vsp += e->vsp;
	}

	if (_play_sound) {
		play_sound(snd_shoot, e->x, e->y);
	}

	return b;
}

static Bullet* shoot_homing(Enemy* e, float dir,
							bool _play_sound = true, bool add_enemy_speed = true) {
	Bullet* b = world->CreateBullet();
	b->x = e->x;
	b->y = e->y;
	b->dmg = 15.0f;

	b->bullet_type = BulletType::HOMING;
	b->dir = dir;
	b->sprite = spr_missile;
	b->lifespan = 10.0f * 60.0f;
	b->max_acc = 0.3f;
	b->max_spd = 10.0f;

	if (add_enemy_speed) {
		b->hsp += e->hsp;
		b->vsp += e->vsp;
	}

	if (_play_sound) {
		play_sound(snd_shoot, e->x, e->y);
	}

	return b;
}

template <typename F>
static void shoot_radial(Enemy* e, int n, float dir_diff, const F& f, bool _play_sound = true) {
	for (int i = 0; i < n; i++) {
		float a = -(float(n) - 1.0f) / 2.0f + float(i);
		Bullet* b = f(i);

		float spd = length(b->hsp, b->vsp);
		float dir = point_direction(0.0f, 0.0f, b->hsp, b->vsp);
		dir += a * dir_diff;
		b->hsp = lengthdir_x(spd, dir);
		b->vsp = lengthdir_y(spd, dir);

		b->hsp += e->hsp;
		b->vsp += e->vsp;
	}

	if (_play_sound) {
		play_sound(snd_shoot, e->x, e->y);
	}
}
