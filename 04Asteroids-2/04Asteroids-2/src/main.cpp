#include "common.h"
#include "window_creation.h"
#include "game.h"
#include "batch_renderer.h"
#include "package.h"


int main(int argc, char* argv[]) {
	init_window_and_opengl("Asteroids", GAME_W, GAME_H, 2, true);
	defer { deinit_window_and_opengl(); };

	init_package();
	defer { deinit_package(); };

	init_renderer();
	defer { deinit_renderer(); };

	Game game = {};

	game.init();
	defer { game.deinit(); };

	while (!should_quit) {
		SDL_Event ev;
		while (SDL_PollEvent(&ev)) {
			handle_event(&ev);
		}

		float delta = 1;

		game.update(delta);

		renderer.draw_calls = 0;
		renderer.max_batch = 0;

		vec4 clear_color = get_color(0x6495edff);
		glClearColor(clear_color.r, clear_color.g, clear_color.b, clear_color.a);
		glClear(GL_COLOR_BUFFER_BIT);

		// scale to fit
		{
			int backbuffer_width;
			int backbuffer_height;
			SDL_GL_GetDrawableSize(window, &backbuffer_width, &backbuffer_height);

			float xscale = backbuffer_width  / (float)GAME_W;
			float yscale = backbuffer_height / (float)GAME_H;
			float scale = min(xscale, yscale);

			int w = (int) (GAME_W * scale);
			int h = (int) (GAME_H * scale);
			int x = (backbuffer_width  - w) / 2;
			int y = (backbuffer_height - h) / 2;

			glViewport(x, y, w, h);
		}

		game.draw(delta);

		break_batch();

		swap_buffers();
	}

	return 0;
}


#define STB_SPRINTF_IMPLEMENTATION
#include <stb/stb_sprintf.h>
#undef STB_SPRINTF_IMPLEMENTATION


#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#undef STB_IMAGE_IMPLEMENTATION


#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>
#undef GLAD_GL_IMPLEMENTATION

