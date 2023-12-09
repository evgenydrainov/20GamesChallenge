#pragma once

#include "Sprite.h"

#define MAX_PARTICLES 1000
#define MAX_PARTICLE_TYPES 16

enum struct PartShape {
	CIRCLE,
	SPRITE,
	TEXT
};

struct Font;

struct PartType {
	PartShape shape;
	float spd_from;
	float spd_to;
	float dir_min;
	float dir_max;
	float lifespan_min;
	float lifespan_max;
	SDL_Color color_from;
	SDL_Color color_to;
	union {
		struct { // CIRCLE
			float radius_from;
			float radius_to;
		};
		struct { // SPRITE
			float xscale_from;
			float xscale_to;
			float yscale_from;
			float yscale_to;
			Sprite* sprite;
		};
		struct { // TEXT
			float xscale_from;
			float xscale_to;
			float yscale_from;
			float yscale_to;
			Font* font;
		};
	};
};

struct Particle {
	int type;
	float x;
	float y;
	float dir;
	float lifetime;
	float lifespan;
	union {
		struct { // SPRITE
			float frame_index;
		};
		struct { // TEXT
			const char* text;
		};
	};
};

struct Particles {
	Particle* particles;
	int particle_count;

	PartType types[MAX_PARTICLE_TYPES];

	void Init();
	void Free();

	void Update(float delta);
	void Draw(float delta);

	void SetTypeCircle(int index,
					   float spd_from, float spd_to,
					   float dir_min, float dir_max,
					   float lifespan_min, float lifespan_max,
					   SDL_Color color_from, SDL_Color color_to,
					   float radius_from, float radius_to);

	void SetTypeSprite(int index,
					   float spd_from, float spd_to,
					   float dir_min, float dir_max,
					   float lifespan_min, float lifespan_max,
					   SDL_Color color_from, SDL_Color color_to,
					   Sprite* sprite,
					   float xscale_from, float xscale_to,
					   float yscale_from, float yscale_to);

	Particle* CreateParticles(float x, float y, int type, int count);

	void DestroyParticleByIndex(int index);
};
