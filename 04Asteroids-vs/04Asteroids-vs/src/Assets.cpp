#include "Assets.h"

#include "Game.h"
#include "Audio.h"

#include <SDL_image.h>
#include <SDL_ttf.h>

Sprite Sprites[SPRITE_COUNT] = {
	/* spr_player_ship */ { nullptr,  0,  0,  48,   48,   24,  24,  1,  1,  0.0f,          0 },
	/* spr_asteroid1   */ { nullptr,  0,  0,  30,   30,   15,  15,  1,  1,  0.0f,          0 },
	/* spr_asteroid2   */ { nullptr,  0,  0,  60,   60,   30,  30,  1,  1,  0.0f,          0 },
	/* spr_asteroid3   */ { nullptr,  0,  0,  110,  110,  55,  55,  1,  1,  0.0f,          0 },
	/* spr_invader     */ { nullptr,  0,  0,  56,   56,   28,  28,  2,  2,  1.0f / 40.0f,  0 }
};

SDL_Texture* Textures[TEXTURE_COUNT];
Font Fonts[FONT_COUNT];
Mix_Chunk* Chunks[SOUND_COUNT];
const char* Chunk_Names[SOUND_COUNT];

bool load_all_assets() {
	SDL_Renderer* renderer = game->renderer;

	bool error = false;

	if (IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) {
		if (!(spr_player_ship->texture = IMG_LoadTexture(renderer, "img/spr_player_ship.png"))) error = true;
		if (!(spr_asteroid1->texture   = IMG_LoadTexture(renderer, "img/spr_asteroid1.png")))   error = true;
		if (!(spr_asteroid2->texture   = IMG_LoadTexture(renderer, "img/spr_asteroid2.png")))   error = true;
		if (!(spr_asteroid3->texture   = IMG_LoadTexture(renderer, "img/spr_asteroid3.png")))   error = true;
		if (!(spr_invader->texture     = IMG_LoadTexture(renderer, "img/spr_invader.png")))     error = true;

		if (!(tex_bg   = IMG_LoadTexture(renderer, "img/tex_bg.png")))   error = true;
		if (!(tex_bg1  = IMG_LoadTexture(renderer, "img/tex_bg1.png")))  error = true;
		if (!(tex_moon = IMG_LoadTexture(renderer, "img/tex_moon.png"))) error = true;
	}
	IMG_Quit();

	if (TTF_Init() == 0) {
		if (!LoadFontFromFileTTF(fnt_mincho, "font/mincho.ttf", 22, renderer)) error = true;
		if (!LoadFontFromFileTTF(fnt_cp437,  "font/cp437.ttf",  16, renderer)) error = true;
	} else {
		error = true;
	}
	TTF_Quit();

	{
		if (!(snd_ship_engine  = Mix_LoadWAV("audio/snd_ship_engine.wav")))  error = true;
		if (!(snd_shoot        = Mix_LoadWAV("audio/snd_shoot.wav")))        error = true;
		if (!(snd_hurt         = Mix_LoadWAV("audio/snd_hurt.wav")))         error = true;
		if (!(snd_explode      = Mix_LoadWAV("audio/snd_explode.wav")))      error = true;
		if (!(snd_boss_explode = Mix_LoadWAV("audio/snd_boss_explode.wav"))) error = true;
		if (!(snd_powerup      = Mix_LoadWAV("audio/snd_powerup.wav")))      error = true;

		int i = 0;
		Chunk_Names[i++] = "snd_ship_engine.wav";
		Chunk_Names[i++] = "snd_shoot.wav";
		Chunk_Names[i++] = "snd_hurt.wav";
		Chunk_Names[i++] = "snd_explode.wav";
		Chunk_Names[i++] = "snd_boss_explode.wav";
		Chunk_Names[i++] = "snd_powerup.wav";
	}

	return !error;
}

void free_all_assets() {
	for (int i = SOUND_COUNT; i--;) {
		Mix_FreeChunk(Chunks[i]);
	}
	for (int i = FONT_COUNT; i--;) {
		DestroyFont(&Fonts[i]);
	}
	for (int i = TEXTURE_COUNT; i--;) {
		SDL_DestroyTexture(Textures[i]);
	}
	for (int i = SPRITE_COUNT; i--;) {
		SDL_DestroyTexture(Sprites[i].texture);
	}
}
