
#include <cstdlib>

#include <iostream>
#include <memory>
#include <vector>

#include <SDL2/SDL.h>

class GameState
{
	// TODO
};

class Game
{
	bool running = false;
	GameState currentState;
	std::shared_ptr<SDL_Window*> window = std::make_shared_for_overwrite<SDL_Window*>();
	std::shared_ptr<SDL_Surface*> surface = std::make_shared_for_overwrite<SDL_Surface*>();

public:
	Game()
	{
		if (SDL_Init(SDL_INIT_VIDEO) < 0) {
			std::cerr << "[Error] Could not initialize SDL: " << SDL_GetError() << "\n";
			std::exit(EXIT_FAILURE);
		}

		window = std::make_shared<SDL_Window*>(SDL_CreateWindow("Minesweeper", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1600, 1200, SDL_WINDOW_SHOWN));
		surface = std::make_shared<SDL_Surface*>(SDL_GetWindowSurface(*window));
	}

	~Game()
	{
		SDL_DestroyWindow(*window);
		SDL_Quit();
	}

	void start()
	{
		running = true;

		while (running) {
			processInput();
			update();
			render();
		}
	}

protected:
	void processInput()
	{
		SDL_Event event;

		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_QUIT: {
					running = false;
				} break;
			}
		}
	}

	void update()
	{
		// TODO: Game logic.
	}

	void render()
	{
		// TODO: Window rendering.
		SDL_FillRect(*surface, nullptr, SDL_MapRGB((*surface)->format, 0xFF, 0xFF, 0xFF));
		SDL_UpdateWindowSurface(*window);

		// TODO: FPS correction to not abuse the CPU.
		SDL_Delay(16);
	}
};

int main(int argc, char *argv[])
{
	Game().start();
	return EXIT_SUCCESS;
}
