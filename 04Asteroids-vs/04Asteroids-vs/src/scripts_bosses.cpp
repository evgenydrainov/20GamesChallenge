#include "scripts_common.h"

#define self ((Enemy*)(co->user_data))

static Bullet* bshoot(Enemy* e, float spd, float dir, bool _play_sound = true, bool add_enemy_speed = true) {
	Bullet* b = shoot(e, spd, dir, _play_sound, add_enemy_speed);
	b->lifespan *= 2.0f;
	// b->radius *= 1.5f;
	return b;
}

static float dir_to_player(mco_coro* co) {
	return point_direction_wrapped(self->x, self->y, world->player.x, world->player.y);
}

void boss0_script(mco_coro* co) {
	float dir = 0.0f;
	while (true) {
		shoot_radial(self, 15, 360.0f / 15.0f, [=](int j) {
			return bshoot(self, 6.5f, dir_to_player(co), false, false);
		}, false);

		for (int i = 10; i--;) {
			bshoot(self, 6.0f, dir);
			bshoot(self, 6.0f, dir + 90.0f,  false);
			bshoot(self, 6.0f, dir + 180.0f, false);
			bshoot(self, 6.0f, dir + 270.0f, false);

			dir += 10.0f;
			wait(co, 10);
		}
	}
}

void boss1_script(mco_coro* co) {
	while (true) {
		shoot_radial(self, 19, 360.0f / 19.0f, [=](int j) {
			return bshoot(self, 4.0f, dir_to_player(co), false, false);
		}, false);

		shoot_radial(self, 21, 360.0f / 21.0f, [=](int j) {
			return bshoot(self, 6.0f, dir_to_player(co), false, false);
		});

		wait(co, 30);
	}
}

void boss2_script(mco_coro* co) {
	float dir = 0.0f;
	float d   = 0.0f;
	while (true) {
		bshoot(self, 5.0f, float(dir), false);
		bshoot(self, 5.0f, float(dir) + 180.0f);

		dir += d;
		d   += 0.5f;
		dir = fmodf(dir, 360.0f);
		d   = fmodf(d,   360.0f);

		wait(co, 1);
	}
}
