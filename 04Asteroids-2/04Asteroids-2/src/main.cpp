#include "common.h"
#include "window_creation.h"
#include "game.h"
#include "batch_renderer.h"
#include "package.h"


int main(int /*argc*/, char* /*argv*/[]) {
	init_window_and_opengl("Asteroids", GAME_W, GAME_H, 2, false);
	defer { deinit_window_and_opengl(); };

	init_package();
	defer { deinit_package(); };

	init_renderer();
	defer { deinit_renderer(); };

	game.init();
	defer { game.deinit(); };

	while (!window.should_quit) {
		begin_frame();

		game.update(window.delta);

		vec4 clear_color = color_black;
		render_begin_frame(clear_color);

		game.draw(window.delta);

		render_end_frame();

		swap_buffers();
	}

	return 0;
}


#pragma warning(push, 0)


#define STB_SPRINTF_IMPLEMENTATION
#include <stb/stb_sprintf.h>
#undef STB_SPRINTF_IMPLEMENTATION


#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_NO_STDIO
// TODO: STBI_ASSERT
#include <stb/stb_image.h>
#undef STB_IMAGE_IMPLEMENTATION


#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>
#undef GLAD_GL_IMPLEMENTATION


#pragma warning(pop)

