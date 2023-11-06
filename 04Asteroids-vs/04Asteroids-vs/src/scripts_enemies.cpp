#include "scripts_common.h"

#define self ((Enemy*)(co->user_data))

void enemy0_script(mco_coro* co) {
	wait(co, random_range(&world->rng, 0, 2 * 60));

	while (true) {
		for (int i = 5; i--;) {
			shoot(self,
				  10.0f,
				  self->angle);

			wait(co, random_range(&world->rng, 8, 12));
		}

		wait(co, 2 * 60);
	}
}
