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

static int play_sound_3d(Mix_Chunk* chunk, float x, float y) {
	float dist = INFINITY;
	auto check_dist = [&dist](float x, float y) {
		float center_x = game->camera_x + (float)GAME_W / 2.0f;
		float center_y = game->camera_y + (float)GAME_H / 2.0f;
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

	float volume = 1.0f - dist / 1000.0f;
	if (volume < 0.0f) volume = 0.0f;
	if (volume > 1.0f) volume = 1.0f;

	if (volume == 0.0f) {
		return -1;
	}

	float pan = (x - game->camera_x) / (float)GAME_W;
	if (pan < 0.0f) pan = 0.0f;
	if (pan > 1.0f) pan = 1.0f;

	float left = 1.0f - (pan - 0.5f) / 0.5f;
	if (left > 1.0f) left = 1.0f;

	float right = pan / 0.5f;
	if (right > 1.0f) right = 1.0f;

	// stop_sound(chunk);

	bool has_free = false;
	for (int i = 0; i < Mix_AllocateChannels(-1); i++) {
		if (!Mix_Playing(i)) {
			has_free = true;
			break;
		}
	}

	if (!has_free) {
		int channels = Mix_AllocateChannels(-1);
		Mix_AllocateChannels(channels + 1);
	}

	int channel = Mix_PlayChannel(-1, chunk, 0);

	Mix_SetPanning(channel, (Uint8)(left * 255.0f), (Uint8)(right * 255.0f));
	Mix_SetDistance(channel, (Uint8)((1.0f - volume) * 255.0f));

	return channel;
}

static int play_sound_2d(Mix_Chunk* chunk, float x, float y) {
	auto check_if_on_screen = [](float x, float y) {
		x -= game->camera_x;
		y -= game->camera_y;
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

	return channel;
}

static int play_sound(Mix_Chunk* chunk, float x, float y) {
	if (game->audio_3d) {
		return play_sound_3d(chunk, x, y);
	} else {
		return play_sound_2d(chunk, x, y);
	}
}
