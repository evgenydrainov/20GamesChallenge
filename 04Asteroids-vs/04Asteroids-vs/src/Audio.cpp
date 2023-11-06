#include "Audio.h"

#include "Game.h"
#include "mathh.h"

u32 channel_when_played[MIX_CHANNELS];
int channel_priority[MIX_CHANNELS];

void stop_sound(Mix_Chunk* chunk) {
	for (int i = 0; i < Mix_AllocateChannels(-1); i++) {
		if (Mix_Playing(i)) {
			if (Mix_GetChunk(i) == chunk) {
				Mix_HaltChannel(i);
			}
		}
	}
}

bool sound_is_playing(Mix_Chunk* chunk) {
	for (int i = 0; i < Mix_AllocateChannels(-1); i++) {
		if (Mix_Playing(i)) {
			if (Mix_GetChunk(i) == chunk) {
				return true;
			}
		}
	}
	return false;
}

int play_sound_3d(Mix_Chunk* chunk, float x, float y, int priority) {
	float center_x = world->camera_x;
	float center_y = world->camera_y;
	float dist = point_distance_wrapped(x, y, center_x, center_y);

	float volume = 1.0f - dist / DIST_OFFSCREEN;
	volume = clamp(volume, 0.0f, 1.0f);

	if (volume == 0.0f) {
		return -1;
	}

	float pan = (x - world->camera_left) / world->camera_w;
	pan = clamp(pan, 0.0f, 1.0f);

	float left = 1.0f - (pan - 0.5f) / 0.5f;
	left = clamp(left, 0.0f, 1.0f);

	float right = pan / 0.5f;
	right = clamp(right, 0.0f, 1.0f);

	{
		auto count_instances_of_this_chunk = [chunk]() {
			int result = 0;
			for (int i = 0; i < Mix_AllocateChannels(-1); i++) {
				if (Mix_Playing(i) && Mix_GetChunk(i) == chunk) {
					result++;
				}
			}
			return result;
		};

		int instances_of_this_chunk = count_instances_of_this_chunk();

		while (instances_of_this_chunk >= 2) {
			double earliest_time = INFINITY;
			int earliest_channel = -1;
			for (int i = 0; i < Mix_AllocateChannels(-1); i++) {
				if (Mix_Playing(i) && Mix_GetChunk(i) == chunk && channel_priority[i] <= priority) {
					if (channel_when_played[i] < earliest_time) {
						earliest_time = channel_when_played[i];
						earliest_channel = i;
					}
				}
			}

			if (earliest_channel == -1) {
				break;
			}

			Mix_HaltChannel(earliest_channel);
			instances_of_this_chunk--;
		}

		if (instances_of_this_chunk >= 2) {
			return -1;
		}
	}

	bool has_free = false;
	for (int i = 0; i < Mix_AllocateChannels(-1); i++) {
		if (!Mix_Playing(i)) {
			has_free = true;
			break;
		}
	}

	if (!has_free) {
		return -1;
	}

	int channel = Mix_PlayChannel(-1, chunk, 0);
	channel_when_played[channel] = SDL_GetTicks();
	channel_priority[channel] = priority;

	Mix_SetPanning(channel, u8(left * 255.0f), u8(right * 255.0f));
	Mix_SetDistance(channel, u8((1.0f - volume) * 255.0f));

	return channel;
}

int play_sound_2d(Mix_Chunk* chunk, float x, float y, int priority) {
	// if (!is_on_screen(x, y)) {
	// 	return -1;
	// }

	float center_x = world->camera_x;
	float center_y = world->camera_y;
	float dist = point_distance_wrapped(x, y, center_x, center_y);
	if (dist > DIST_OFFSCREEN) {
		return -1;
	}

	stop_sound(chunk);

	bool has_free = false;
	for (int i = 0; i < Mix_AllocateChannels(-1); i++) {
		if (!Mix_Playing(i)) {
			has_free = true;
			break;
		}
	}

	if (!has_free) {
		return -1;
	}

	int channel = Mix_PlayChannel(-1, chunk, 0);
	channel_when_played[channel] = SDL_GetTicks();
	channel_priority[channel] = priority;

	return channel;
}

int play_sound(Mix_Chunk* chunk, float x, float y, int priority) {
	if (game->options.audio_3d) {
		return play_sound_3d(chunk, x, y, priority);
	} else {
		return play_sound_2d(chunk, x, y, priority);
	}
}
