#define ArrayLength(a) (sizeof(a) / sizeof(*a))

static double GetTime() {
	return (double)SDL_GetPerformanceCounter() / (double)SDL_GetPerformanceFrequency();
}

static void stop_sound(Mix_Chunk* chunk) {
	for (int i = 0; i < Mix_AllocateChannels(-1); i++) {
		if (Mix_Playing(i)) {
			if (Mix_GetChunk(i) == chunk) {
				Mix_HaltChannel(i);
			}
		}
	}
}

static bool sound_playing(Mix_Chunk* chunk) {
	for (int i = 0; i < Mix_AllocateChannels(-1); i++) {
		if (Mix_Playing(i)) {
			if (Mix_GetChunk(i) == chunk) {
				return true;
			}
		}
	}
	return false;
}

static int play_sound_3d(Mix_Chunk* chunk, float x, float y, int priority = 0) {
	float dist = INFINITY;
	auto check_dist = [&dist](float x, float y) {
		float center_x = world->camera_x + (float)GAME_W / 2.0f;
		float center_y = world->camera_y + (float)GAME_H / 2.0f;
		float dist1 = point_distance(center_x, center_y, x, y);
		if (dist1 < dist) dist = dist1;
	};
	check_dist(x - MAP_W, y - MAP_H);
	check_dist(x,         y - MAP_H);
	check_dist(x + MAP_W, y - MAP_H);

	check_dist(x - MAP_W, y);
	check_dist(x,         y);
	check_dist(x + MAP_W, y);

	check_dist(x - MAP_W, y + MAP_H);
	check_dist(x,         y + MAP_H);
	check_dist(x + MAP_W, y + MAP_H);

	float volume = 1.0f - dist / 800.0f;
	if (volume < 0.0f) volume = 0.0f;
	if (volume > 1.0f) volume = 1.0f;

	if (volume == 0.0f) {
		return -1;
	}

	float pan = (x - world->camera_x) / (float)GAME_W;
	if (pan < 0.0f) pan = 0.0f;
	if (pan > 1.0f) pan = 1.0f;

	float left = 1.0f - (pan - 0.5f) / 0.5f;
	if (left > 1.0f) left = 1.0f;

	float right = pan / 0.5f;
	if (right > 1.0f) right = 1.0f;

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
	channel_when_played[channel] = game->time * (1000.0f / 60.0f);
	channel_priority[channel] = priority;

	Mix_SetPanning(channel, (Uint8)(left * 255.0f), (Uint8)(right * 255.0f));
	Mix_SetDistance(channel, (Uint8)((1.0f - volume) * 255.0f));

	return channel;
}

static int play_sound_2d(Mix_Chunk* chunk, float x, float y, int priority = 0) {
	auto check_if_on_screen = [](float x, float y) {
		x -= world->camera_x;
		y -= world->camera_y;
		return (-100.0f <= x && x < (float)GAME_W + 100.0f)
			&& (-100.0f <= y && y < (float)GAME_H + 100.0f);
	};

	bool is_on_screen = false;
	is_on_screen |= check_if_on_screen(x - MAP_W, y - MAP_H);
	is_on_screen |= check_if_on_screen(x,         y - MAP_H);
	is_on_screen |= check_if_on_screen(x + MAP_W, y - MAP_H);

	is_on_screen |= check_if_on_screen(x - MAP_W, y);
	is_on_screen |= check_if_on_screen(x,         y);
	is_on_screen |= check_if_on_screen(x + MAP_W, y);

	is_on_screen |= check_if_on_screen(x - MAP_W, y + MAP_H);
	is_on_screen |= check_if_on_screen(x,         y + MAP_H);
	is_on_screen |= check_if_on_screen(x + MAP_W, y + MAP_H);

	if (!is_on_screen) {
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
	channel_when_played[channel] = game->time * (1000.0f / 60.0f);
	channel_priority[channel] = priority;

	return channel;
}

static int play_sound(Mix_Chunk* chunk, float x, float y, int priority = 0) {
	if (game->audio_3d) {
		return play_sound_3d(chunk, x, y, priority);
	} else {
		return play_sound_2d(chunk, x, y, priority);
	}
}
