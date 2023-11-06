#include "Sprite.h"

#include "Game.h"

float sprite_get_next_frame_index(Sprite* sprite, float frame_index, float delta) {
	if (!sprite) return frame_index;

	int frame_count = sprite->frame_count;
	int loop_frame  = sprite->loop_frame;
	float anim_spd  = sprite->anim_spd;

	frame_index += anim_spd * delta;
	if (int(frame_index) >= frame_count) {
		frame_index = float(loop_frame) + SDL_fmodf(frame_index - float(loop_frame), float(frame_count - loop_frame));
	}
	return frame_index;
}

static double AngleToSDL(float angle) {
	return (double) (-angle);
}

void DrawSprite(Sprite* sprite, int frame_index,
				float x, float y,
				float angle, float xscale, float yscale,
				SDL_Color color) {
	if (!sprite) return;
	if (!sprite->texture) return;
	if (sprite->frames_in_row == 0) return;

	if (frame_index > sprite->frame_count - 1) frame_index = sprite->frame_count - 1;
	if (frame_index < 0) frame_index = 0;

	int cell_x = frame_index % sprite->frames_in_row;
	int cell_y = frame_index / sprite->frames_in_row;

	SDL_Rect src;
	src.x = sprite->u + cell_x * sprite->width;
	src.y = sprite->v + cell_y * sprite->height;
	src.w = sprite->width;
	src.h = sprite->height;

	SDL_FRect dest;
	dest.x = x - float(sprite->xorigin) * fabsf(xscale);
	dest.y = y - float(sprite->yorigin) * fabsf(yscale);
	dest.w = float(sprite->width)  * fabsf(xscale);
	dest.h = float(sprite->height) * fabsf(yscale);

	int flip = SDL_FLIP_NONE;
	if (xscale < 0.0f) flip |= SDL_FLIP_HORIZONTAL;
	if (yscale < 0.0f) flip |= SDL_FLIP_VERTICAL;

	SDL_FPoint center;
	center.x = float(sprite->xorigin) * fabsf(xscale);
	center.y = float(sprite->yorigin) * fabsf(yscale);

	SDL_Renderer* renderer = game->renderer;

	SDL_SetTextureColorMod(sprite->texture, color.r, color.g, color.b);
	SDL_SetTextureAlphaMod(sprite->texture, color.a);
	SDL_RenderCopyExF(renderer, sprite->texture, &src, &dest, AngleToSDL(angle), &center, (SDL_RendererFlip) flip);
}
