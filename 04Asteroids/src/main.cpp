//
// https://20_games_challenge.gitlab.io/games/asteroids/
//

#include "Game.h"

#define MINICORO_IMPL
#include "minicoro.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#ifdef __EMSCRIPTEN__
static void emscripten_main_loop() {
	game->Frame();
}
#endif

int main() {
	Game game_instance = {};
	game = &game_instance;

	game->Init();

#ifdef __EMSCRIPTEN__
	emscripten_set_main_loop(emscripten_main_loop, 60, 1);
#else
	game->Run();
#endif

	game->Quit();

	return 0;
}
