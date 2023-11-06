#pragma once

#include "common.h"
#include <SDL_mixer.h>

extern u32 channel_when_played[MIX_CHANNELS];
extern int channel_priority[MIX_CHANNELS];

void stop_sound(Mix_Chunk* chunk);
bool sound_is_playing(Mix_Chunk* chunk);
int play_sound_3d(Mix_Chunk* chunk, float x, float y, int priority = 0);
int play_sound_2d(Mix_Chunk* chunk, float x, float y, int priority = 0);
int play_sound(Mix_Chunk* chunk, float x, float y, int priority = 0);
