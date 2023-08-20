static Bullet* enemy_shoot(Enemy* e, float spd, float dir) {
	Bullet* b = game->CreateBullet();
	b->x = e->x;
	b->y = e->y;
	b->hsp = e->hsp + lengthdir_x(spd, dir);
	b->vsp = e->vsp + lengthdir_y(spd, dir);

	play_sound(game->snd_shoot, e->x, e->y);

	return b;
}

static Bullet* _enemy_shoot(Enemy* e, float spd, float dir) {
	Bullet* b = game->CreateBullet();
	b->x = e->x;
	b->y = e->y;
	b->hsp = lengthdir_x(spd, dir);
	b->vsp = lengthdir_y(spd, dir);

	play_sound(game->snd_shoot, e->x, e->y);

	return b;
}

#define self ((Enemy*)(co->user_data))
#define shoot enemy_shoot
#define _shoot _enemy_shoot

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

#undef self
#undef shoot
#undef _shoot
