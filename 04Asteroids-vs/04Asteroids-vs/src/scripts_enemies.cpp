#include "scripts_common.h"

#define self ((Enemy*)(co->user_data))

void script_enemy(mco_coro* co) {
	int t = random_range(&world->rng, 8, 12);

	wait(co, random_range(&world->rng, 0, 2 * 60));

	while (true) {
		for (int i = 5; i--;) {
			shoot(self,
				  10.0f,
				  self->angle);

			wait(co, t);
		}

		wait(co, 2 * 60);
	}
}

void script_enemy_spread(mco_coro* co) {
	wait(co, random_range(&world->rng, 0, 2 * 60));

	while (true) {
		for (int i = 5; i--;) {
			shoot_radial(self, 4, 12.0f, [=](int j) {
				return shoot(self, 9.0f, self->angle, false, false);
			});

			wait(co, 18);
		}

		wait(co, 2 * 60);
	}
}

void script_enemy_missile(mco_coro* co) {
	wait(co, random_range(&world->rng, 0, 2 * 60));

	while (true) {
		shoot_homing(self,
					 self->angle);

		wait(co, 2 * 60);
	}
}
