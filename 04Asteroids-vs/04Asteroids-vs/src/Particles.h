#pragma once

#include "Sprite.h"

#define MAX_PARTICLES 1000

enum struct ParticleType {
	CIRCLE,
	SPRITE
};

struct Particle {
	ParticleType type;
	float x;
	float y;
	float hsp;
	float vsp;
	float hacc;
	float vacc;
	float lifetime;
	float lifespan;
	SDL_Color color;
	union {
		struct { // CIRCLE
			float radius;
		};
		struct { // SPRITE
			Sprite* sprite;
			float frame_index;
			float xscale;
			float yscale;
			float angle;
		};
	};
};

struct Particles {
	Particle* particles;
	int particle_count;

	void Init();
	void Free();

	void Update(float delta);
	void Draw(float delta);

	Particle* CreateParticle();

	Particle* CreateCircle(float x, float y,
						   float hsp, float vsp,
						   float hacc, float vacc,
						   float radius,
						   SDL_Color color,
						   float lifespan);

	Particle* CreateSprite(float x, float y,
						   float hsp, float vsp,
						   float hacc, float vacc,
						   Sprite* sprite,
						   float xscale,
						   float yscale,
						   float angle,
						   SDL_Color color,
						   float lifespan);

	void DestroyParticleByIndex(int index);
};
