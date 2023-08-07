#include <SDL2/SDL.h>

#define GAME_FPS 60
#define GAME_W 640
#define GAME_H 480

#define PADDLE_LENGTH 50
#define PADDLE_THICK 10
#define PLAYER_X (GAME_W * 9 / 10)
#define COMPUTER_X (GAME_W / 10)

enum {
	WAITING_TO_SERVE
};

struct Paddle {
	int y;
};

struct Game {
	Paddle p_player;
	Paddle p_computer;
	int state;

	SDL_Window* window;
	SDL_Renderer* renderer;

	void Init();
	void Run();
	void Quit();
};

Game* game;

void Game::Init() {
	SDL_Init(SDL_INIT_VIDEO);

	window = SDL_CreateWindow("01Pong",
							  SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
							  GAME_W, GAME_H,
							  SDL_WINDOW_RESIZABLE);

	renderer = SDL_CreateRenderer(window, -1,
								  SDL_RENDERER_ACCELERATED
								  | SDL_RENDERER_TARGETTEXTURE);

	SDL_RenderSetLogicalSize(renderer, GAME_W, GAME_H);
}

static double GetTime() {
	return (double)SDL_GetPerformanceCounter() / (double)SDL_GetPerformanceFrequency();
}

void Game::Run() {
	double prev_time = GetTime();

	bool quit = false;
	while (!quit) {
		double time = GetTime();

		double frame_end_time = time + (1.0 / (double)GAME_FPS);

		double fps = 1.0 / (time - prev_time);
		prev_time = time;

		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_QUIT: {
					quit = true;
					break;
				}
			}
		}

		printf("%f\n", fps);

		// update
		{
			int mouse_x;
			int mouse_y;
			SDL_GetMouseState(&mouse_x, &mouse_y);

			float logical_x;
			float logical_y;
			SDL_RenderWindowToLogical(renderer, mouse_x, mouse_y, &logical_x, &logical_y);

			p_player.y = (int)logical_y;

			switch (state) {
				case WAITING_TO_SERVE: {

					break;
				}
			}
		}

		// draw
		{
			SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
			SDL_RenderClear(renderer);

			// draw player paddle
			{
				SDL_Rect rect = {
					PLAYER_X - PADDLE_THICK / 2,
					p_player.y - PADDLE_LENGTH / 2,
					PADDLE_THICK,
					PADDLE_LENGTH
				};
				SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
				SDL_RenderFillRect(renderer, &rect);
			}

			// draw computer paddle
			{
				SDL_Rect rect = {
					COMPUTER_X - PADDLE_THICK / 2,
					p_computer.y - PADDLE_LENGTH / 2,
					PADDLE_THICK,
					PADDLE_LENGTH
				};
				SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
				SDL_RenderFillRect(renderer, &rect);
			}

			SDL_RenderPresent(renderer);
		}

		time = GetTime();
		double time_left = frame_end_time - time;
		if (time_left > 0.0) {
			SDL_Delay((Uint32)(time_left * 0.95));
			while (GetTime() < frame_end_time) {}
		}
	}
}

void Game::Quit() {
	SDL_DestroyRenderer(renderer);
	renderer = nullptr;

	SDL_DestroyWindow(window);
	window = nullptr;

	SDL_Quit();
}

int main(int argc, char* argv[]) {
	Game _game = {};
	game = &_game;

	game->Init();
	game->Run();
	game->Quit();

	return 0;
}
