#include "Sprite.h"

#include "Game.h"

#ifdef _WIN32
#include <SDL_image.h>
#else
#include <SDL2/SDL_image.h>
#endif

#define SPRITE_GROUP_ATLAS_W 1024
#define SPRITE_GROUP_ATLAS_H 1024

static double AngleToSDL(float angle) {
	return -(double)angle;
}

void DrawSprite(Sprite* sprite, int frame_index,
				float x, float y,
				float angle, float xscale, float yscale,
				SDL_Color color) {
	if (!sprite->texture) {
		sprite->texture = sprite->group->atlas_texture[sprite->group_index];
	}

	if (frame_index < 0) frame_index = 0;
	if (frame_index > sprite->frame_count - 1) frame_index = sprite->frame_count - 1;

	int cell_x = frame_index % sprite->frames_in_row;
	int cell_y = frame_index / sprite->frames_in_row;

	SDL_Rect src;
	src.x = sprite->u + cell_x * sprite->width;
	src.y = sprite->v + cell_y * sprite->height;
	src.w = sprite->width;
	src.h = sprite->height;

	SDL_FRect dest;
	dest.x = (x - (float)sprite->xorigin * fabsf(xscale));
	dest.y = (y - (float)sprite->yorigin * fabsf(yscale));
	dest.w = ((float)sprite->width  * fabsf(xscale));
	dest.h = ((float)sprite->height * fabsf(yscale));

	int flip = SDL_FLIP_NONE;
	if (xscale < 0.0f) flip |= SDL_FLIP_HORIZONTAL;
	if (yscale < 0.0f) flip |= SDL_FLIP_VERTICAL;

	SDL_FPoint center;
	center.x = ((float)sprite->xorigin * fabsf(xscale));
	center.y = ((float)sprite->yorigin * fabsf(yscale));

	SDL_SetTextureColorMod(sprite->texture, color.r, color.g, color.b);
	SDL_SetTextureAlphaMod(sprite->texture, color.a);
	SDL_RenderCopyExF(game->renderer, sprite->texture, &src, &dest, AngleToSDL(angle), &center, (SDL_RendererFlip) flip);
}

void LoadSprite(Sprite* sprite, SpriteGroup* group, const char* fname,
				int frame_count, int frames_in_row, float anim_spd, int loop_frame,
				int xorigin, int yorigin) {
	sprite->frame_count = frame_count;
	sprite->frames_in_row = frames_in_row;
	sprite->anim_spd = anim_spd;
	sprite->loop_frame = loop_frame;

	sprite->group = group;

	SDL_Surface* surf = IMG_Load(fname);

	if (group->u + surf->w >= SPRITE_GROUP_ATLAS_W) {
		group->u = 0;
		group->v += group->h;
		group->h = 0;

		if (group->v + surf->h >= SPRITE_GROUP_ATLAS_H) {
			if (group->atlas_count == SPRITE_GROUP_MAX_TEXTURES) {
				SDL_Log("no room for sprite \"%s\"", fname);
				SDL_FreeSurface(surf);
				return;
			}

			group->atlas_surf[group->atlas_count] = SDL_CreateRGBSurfaceWithFormat(0, SPRITE_GROUP_ATLAS_W, SPRITE_GROUP_ATLAS_H, 32, SDL_PIXELFORMAT_ARGB8888);
			group->atlas_count++;

			group->u = 0;
			group->v = 0;
			group->h = 0;
		}
	}

	SDL_Rect dest = {group->u, group->v, surf->w, surf->h};
	SDL_BlitSurface(surf, nullptr, group->atlas_surf[group->atlas_count - 1], &dest);
	if (surf->h > group->h) group->h = surf->h;
	
	sprite->u = group->u;
	sprite->v = group->v;
	sprite->width = surf->w / frames_in_row;
	sprite->height = surf->h / ((frame_count + frames_in_row - 1) / frames_in_row); // ceil
	sprite->xorigin = (xorigin == INT_MAX) ? (sprite->width  / 2) : xorigin;
	sprite->yorigin = (yorigin == INT_MAX) ? (sprite->height / 2) : yorigin;
	
	sprite->group_index = group->atlas_count - 1;

	group->u += surf->w;

	SDL_FreeSurface(surf);
}

void FinalizeSpriteGroup(SpriteGroup* group) {
	for (int i = 0; i < group->atlas_count; i++) {
		SDL_Surface* surf = group->atlas_surf[i];

		group->atlas_texture[i] = SDL_CreateTextureFromSurface(game->renderer, surf);

		SDL_FreeSurface(surf);
	}
}

void InitSpriteGroup(SpriteGroup* group) {
	group->atlas_surf[0] = SDL_CreateRGBSurfaceWithFormat(0, SPRITE_GROUP_ATLAS_W, SPRITE_GROUP_ATLAS_H, 32, SDL_PIXELFORMAT_ARGB8888);

	group->atlas_count = 1;
}

void DestroySpriteGroup(SpriteGroup* group) {
	for (int i = 0; i < group->atlas_count; i++) {
		SDL_DestroyTexture(group->atlas_texture[i]);
		group->atlas_texture[i] = nullptr;
	}
	group->atlas_count = 0;
}

void AnimateSprite(Sprite* sprite, float* frame_index, float delta) {
	int frame_count = sprite->frame_count;
	int loop_frame  = sprite->loop_frame;
	float anim_spd  = sprite->anim_spd;

	*frame_index += anim_spd * delta;
	if ((int)(*frame_index) >= frame_count) {
		*frame_index = (float)loop_frame + SDL_fmodf(*frame_index - (float)loop_frame, (float)(frame_count - loop_frame));
	}
}
