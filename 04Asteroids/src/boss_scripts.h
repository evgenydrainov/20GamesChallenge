static void invader_script2(mco_coro* co) {
	float dir = 0.0f;
	while (true) {
		shoot_radial(co, 15, 360.0f / 15.0f, [](mco_coro* co, int j) {
			Bullet* b = _shoot(self, 6.5f, point_direction(self->x, self->y, world->player.x, world->player.y));
			b->lifespan *= 2.0f;
			return b;
		});

		for (int i = 10; i--;) {
			shoot(self, 6.0f, dir)->lifespan *= 2.0f;
			shoot(self, 6.0f, dir + 90.0f)->lifespan *= 2.0f;
			shoot(self, 6.0f, dir + 180.0f)->lifespan *= 2.0f;
			shoot(self, 6.0f, dir + 270.0f)->lifespan *= 2.0f;

			dir += 10.0f;
			wait(co, 10);
		}
	}
}

static void invader_script3(mco_coro* co) {
	while (true) {
		shoot_radial(co, 19, 360.0f / 19.0f, [](mco_coro* co, int j) {
			Bullet* b = _shoot(self, 4.0f, point_direction(self->x, self->y, world->player.x, world->player.y));
			b->lifespan *= 2.0f;
			return b;
		});

		shoot_radial(co, 21, 360.0f / 21.0f, [](mco_coro* co, int j) {
			Bullet* b = _shoot(self, 6.0f, point_direction(self->x, self->y, world->player.x, world->player.y));
			b->lifespan *= 2.0f;
			return b;
		});

		wait(co, 30);
	}
}

static void invader_script4(mco_coro* co) {
	float dir = 0.0f;
	float d   = 0.0f;
	while (true) {
		shoot(self, 5.0f, (float)dir)->lifespan *= 2.0f;
		shoot(self, 5.0f, (float)dir + 180.0f)->lifespan *= 2.0f;

		dir += d;
		d   += 0.5f;
		dir = SDL_fmodf(dir, 360.0f);
		d   = SDL_fmodf(d,   360.0f);

		wait(co, 1);
	}
}
