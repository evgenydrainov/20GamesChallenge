//
// https://20_games_challenge.gitlab.io/games/asteroids/
//

#include "Game.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

static void emscripten_main_loop() {
	game->Frame();
}
#endif

int main(int argc, char* argv[]) {
	Game game_instance{};
	game = &game_instance;

	game->Init();

#ifdef __EMSCRIPTEN__
	emscripten_set_main_loop(emscripten_main_loop, -1, 1);
#else
	game->Run();
#endif

	game->Quit();

	return 0;
}

#define MINICORO_IMPL
#define MCO_LOG(s) SDL_Log(s)

#ifdef NDEBUG
#define MCO_DEFAULT_STACK_SIZE 16384
#define MCO_MIN_STACK_SIZE 16384
#else
#define MCO_DEFAULT_STACK_SIZE 32768
#define MCO_MIN_STACK_SIZE 32768
#endif

#include "minicoro.h"

#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"
