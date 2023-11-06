#include "Particles.h"

#include "Game.h"
#include "ecalloc.h"

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

		p->hsp += p->hacc * delta;
		p->vsp += p->vacc * delta;

		p->x += p->hsp * delta;
		p->y += p->vsp * delta;

		switch (p->type) {
			case ParticleType::SPRITE: {
				p->frame_index = sprite_get_next_frame_index(p->sprite, p->frame_index, delta);
				break;
			}
		}
	}
}

void Particles::Draw(float delta) {
	SDL_Renderer* renderer = game->renderer;

	for (int i = 0; i < particle_count; i++) {
		Particle* p = &particles[i];

		switch (p->type) {
			case ParticleType::CIRCLE: {
				DrawCircleCamWarped(p->x, p->y, p->radius, p->color);
				break;
			}

			case ParticleType::SPRITE: {
				DrawSpriteCamWarped(p->sprite, int(p->frame_index),
									p->x, p->y,
									p->angle,
									p->xscale, p->yscale,
									p->color);
				break;
			}
		}
	}
}

Particle* Particles::CreateParticle() {
	if (particle_count == MAX_PARTICLES) {
		SDL_Log("Particle limit hit.");
		DestroyParticleByIndex(0);
	}

	Particle* result = &particles[particle_count];
	*result = {};
	particle_count++;

	return result;
}

Particle* Particles::CreateCircle(float x, float y,
								  float hsp, float vsp,
								  float hacc, float vacc,
								  float radius,
								  SDL_Color color,
								  float lifespan) {
	Particle* result = CreateParticle();
	result->type = ParticleType::CIRCLE;

	result->x = x;
	result->y = y;
	result->hsp = hsp;
	result->vsp = vsp;
	result->hacc = hacc;
	result->vacc = vacc;
	result->color = color;
	result->lifespan = lifespan;

	result->radius = radius;

	return result;
}

Particle* Particles::CreateSprite(float x, float y,
								  float hsp, float vsp,
								  float hacc, float vacc,
								  Sprite* sprite,
								  float xscale,
								  float yscale,
								  float angle,
								  SDL_Color color,
								  float lifespan) {
	Particle* result = CreateParticle();
	result->type = ParticleType::SPRITE;

	result->x = x;
	result->y = y;
	result->hsp = hsp;
	result->vsp = vsp;
	result->hacc = hacc;
	result->vacc = vacc;
	result->color = color;
	result->lifespan = lifespan;

	result->sprite = sprite;
	result->xscale = xscale;
	result->yscale = yscale;
	result->angle = angle;

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
