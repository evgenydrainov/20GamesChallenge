static void enemy_ship(mco_coro* co) {
	wait(co, game->random.next() % (2 * 60));

	while (true) {
		for (int i = 5; i--;) {
			shoot(self,
				  10.0f,
				  self->angle);
			
			wait(co, 10);
		}

		wait(co, 2 * 60);
	}
}
