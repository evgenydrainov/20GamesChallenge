#pragma once

#include "Sprite.h"
#include "Font.h"
#include <SDL_mixer.h>

#define SPRITE_COUNT 5
extern Sprite Sprites[SPRITE_COUNT];
#define spr_player_ship (&Sprites[0])
#define spr_asteroid1   (&Sprites[1])
#define spr_asteroid2   (&Sprites[2])
#define spr_asteroid3   (&Sprites[3])
#define spr_invader     (&Sprites[4])

#define TEXTURE_COUNT 3
extern SDL_Texture* Textures[TEXTURE_COUNT];
#define tex_bg   (Textures[0])
#define tex_bg1  (Textures[1])
#define tex_moon (Textures[2])

#define FONT_COUNT 2
extern Font Fonts[FONT_COUNT];
#define fnt_mincho (&Fonts[0])
#define fnt_cp437  (&Fonts[1])

#define SOUND_COUNT 6
extern Mix_Chunk* Chunks[SOUND_COUNT];
extern const char* Chunk_Names[SOUND_COUNT];
#define snd_ship_engine  (Chunks[0])
#define snd_shoot        (Chunks[1])
#define snd_hurt         (Chunks[2])
#define snd_explode      (Chunks[3])
#define snd_boss_explode (Chunks[4])
#define snd_powerup      (Chunks[5])

bool load_all_assets();
void free_all_assets();
