#include "Particles.h"

#include "Game.h"
#include "ecalloc.h"
#include "mathh.h"

void Particles::Init() {
	particles = (Particle*) ecalloc(MAX_PARTICLES, sizeof(*particles));
}

void Particles::Free() {
	free(particles);
}

void Particles::Update(float delta) {
	for (int i = 0; i < particle_count; i++) {
		Particle* p = &particles[i];

		p->lifetime += delta;
		if (p->lifetime > p->lifespan) {
			DestroyParticleByIndex(i);
			i--;
			continue;
		}

		PartType* type = &types[p->type];
		float t = p->lifetime / p->lifespan;

		float spd = lerp(type->spd_from, type->spd_to, t);

		p->x += lengthdir_x(spd, p->dir) * delta;
		p->y += lengthdir_y(spd, p->dir) * delta;

		switch (type->shape) {
			case PartShape::SPRITE: {
				p->frame_index = sprite_get_next_frame_index(type->sprite, p->frame_index, delta);
				break;
			}
		}
	}
}

void Particles::Draw(float delta) {
	SDL_Renderer* renderer = game->renderer;

	for (int i = 0; i < particle_count; i++) {
		Particle* p = &particles[i];

		PartType* type = &types[p->type];
		float t = p->lifetime / p->lifespan;

		float r = lerp(float(type->color_from.r), float(type->color_to.r), t);
		float g = lerp(float(type->color_from.g), float(type->color_to.g), t);
		float b = lerp(float(type->color_from.b), float(type->color_to.b), t);
		float a = lerp(float(type->color_from.a), float(type->color_to.a), t);
		SDL_Color color = {u8(r), u8(g), u8(b), u8(a)};

		switch (type->shape) {
			case PartShape::CIRCLE: {
				float radius = lerp(type->radius_from, type->radius_to, t);
				DrawCircleCamWarped(p->x, p->y, radius, color);
				break;
			}

			case PartShape::SPRITE: {
				float xscale = lerp(type->xscale_from, type->xscale_to, t);
				float yscale = lerp(type->yscale_from, type->yscale_to, t);
				DrawSpriteCamWarped(type->sprite, int(p->frame_index),
									p->x, p->y,
									p->dir,
									xscale, yscale,
									color);
				break;
			}
		}
	}
}

void Particles::SetTypeCircle(int index,
							  float spd_from, float spd_to,
							  float dir_min, float dir_max,
							  float lifespan_min, float lifespan_max,
							  SDL_Color color_from, SDL_Color color_to,
							  float radius_from, float radius_to) {
	if (index < 0 || index >= MAX_PARTICLE_TYPES) {
		return;
	}
	
	PartType* type = &types[index];
	*type = {};

	type->shape = PartShape::CIRCLE;
	type->spd_from = spd_from;
	type->spd_to = spd_to;
	type->dir_min = dir_min;
	type->dir_max = dir_max;
	type->lifespan_min = lifespan_min;
	type->lifespan_max = lifespan_max;
	type->color_from = color_from;
	type->color_to = color_to;
	type->radius_from = radius_from;
	type->radius_to = radius_to;
}

void Particles::SetTypeSprite(int index,
							  float spd_from, float spd_to,
							  float dir_min, float dir_max,
							  float lifespan_min, float lifespan_max,
							  SDL_Color color_from, SDL_Color color_to,
							  Sprite* sprite,
							  float xscale_from, float xscale_to,
							  float yscale_from, float yscale_to) {
	if (index < 0 || index >= MAX_PARTICLE_TYPES) {
		return;
	}

	PartType* type = &types[index];
	*type = {};

	type->shape = PartShape::SPRITE;
	type->spd_from = spd_from;
	type->spd_to = spd_to;
	type->dir_min = dir_min;
	type->dir_max = dir_max;
	type->lifespan_min = lifespan_min;
	type->lifespan_max = lifespan_max;
	type->color_from = color_from;
	type->color_to = color_to;
	type->sprite = sprite;
	type->xscale_from = xscale_from;
	type->xscale_to = xscale_to;
	type->yscale_from = yscale_from;
	type->yscale_to = yscale_to;
}

Particle* Particles::CreateParticles(float x, float y, int type, int count) {
	Particle* result = nullptr;

	if (type < 0 || type >= MAX_PARTICLE_TYPES) {
		return result;
	}

	while (count--) {
		if (particle_count == MAX_PARTICLES) {
			SDL_Log("Particle limit hit.");
			DestroyParticleByIndex(0);
		}

		result = &particles[particle_count];
		*result = {};
		result->type = type;
		result->x = x;
		result->y = y;

		PartType* t = &types[type];

		result->lifespan = random_range(&world->rng_visual, t->lifespan_min, t->lifespan_max);
		result->dir = random_range(&world->rng_visual, t->dir_min, t->dir_max);

		// switch (t->shape) {
		// 	case PartShape::CIRCLE: {
		// 		
		// 		break;
		// 	}
		// 	case PartShape::SPRITE: {
		// 
		// 		break;
		// 	}
		// }

		particle_count++;
	}

	return result;
}

void Particles::DestroyParticleByIndex(int index) {
	if (particle_count == 0) {
		return;
	}

	for (int i = index + 1; i < particle_count; i++) {
		particles[i - 1] = particles[i];
	}
	particle_count--;
}
