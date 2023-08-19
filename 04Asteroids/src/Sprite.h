#pragma once

#ifdef _WIN32
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif

#include <limits.h>

#define SPRITE_GROUP_MAX_TEXTURES 10

struct SpriteGroup {
	union {
		SDL_Texture* atlas_texture[SPRITE_GROUP_MAX_TEXTURES];
		SDL_Surface* atlas_surf[SPRITE_GROUP_MAX_TEXTURES];
	};
	int atlas_count;
	int u;
	int v;
	int h;
};

struct Sprite {
	SDL_Texture* texture;
	int u;
	int v;
	int width;
	int height;
	int xorigin;
	int yorigin;
	int frame_count;
	int frames_in_row;
	float anim_spd;
	int loop_frame;
	int border;

	SpriteGroup* group;
	int group_index;
};

void DrawSprite(Sprite* sprite, int frame_index,
				float x, float y,
				float angle = 0.0f, float xscale = 1.0f, float yscale = 1.0f,
				SDL_Color color = {255, 255, 255, 255});

void LoadSprite(Sprite* sprite, SpriteGroup* group, const char* fname,
				int frame_count = 1, int frames_in_row = 1, float anim_spd = 0.0f, int loop_frame = 0,
				int xorigin = INT_MAX, int yorigin = INT_MAX);

void FinalizeSpriteGroup(SpriteGroup* group);

void InitSpriteGroup(SpriteGroup* group);

void DestroySpriteGroup(SpriteGroup* group);

void AnimateSprite(Sprite* sprite, float* frame_index, float delta);
