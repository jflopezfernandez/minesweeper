
#include <cstdlib>

#include <iostream>
#include <memory>
#include <sstream>
#include <vector>

#include <SDL2/SDL.h>

SDL_Renderer* renderer = nullptr;

SDL_Color blue = { 0x00, 0x00, 0xFF, 0xFF };
SDL_Color red = { 0xFF, 0x00, 0x00, 0xFF };

namespace Configuration
{
	size_t TileSize = 16;
} /** Namespace Configuration */

class IGameObject
{
public:
	virtual void render() = 0;
};

class Tile : IGameObject
{
	int x;
	int y;
	
	bool clicked = false;

public:
	Tile(int x, int y)
		: x { x }, y { y }
	{
		//
	}

	void click()
	{
		clicked = !clicked;
	}

	void render()
	{
		SDL_Rect rect = { x * Configuration::TileSize, y * Configuration::TileSize, Configuration::TileSize, Configuration::TileSize };
		//SDL_SetRenderDrawColor(renderer, (x * Configuration::TileSize) % 256, (y * Configuration::TileSize) % 256, ((x * 8) + (y * 8)) % 256, 0xFF);
		SDL_SetRenderDrawColor(renderer, 0xFF * static_cast<int>(clicked), 0xFF, 0x00, 0xFF);
		SDL_RenderFillRect(renderer, &rect);

		/** Draw the outline of the tile */
		SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
		SDL_RenderDrawRect(renderer, &rect);
	}
};

class Map : IGameObject
{
public:
	size_t tiles_wide;
	size_t tiles_high;
	size_t tile_size;

	std::vector<std::vector<Tile>> tiles;

public:
	
	Map(size_t tiles_wide = 20, size_t tiles_high = 20, size_t tile_size = Configuration::TileSize)
		: tiles_wide { tiles_wide }, tiles_high { tiles_high }, tile_size { tile_size }
	{
		for (auto h = 0; h < tiles_high; ++h) {
			std::vector<Tile> row { };

			for (auto w = 0; w < tiles_wide; ++w) {
				row.emplace_back(Tile(w, h));
			}

			tiles.push_back(row);
		}
	}

	void click(int x, int y)
	{
		tiles[y][x].click();
	}

	void render()
	{
		for (auto& row : tiles) {
			for (auto& tile : row) {
				tile.render();
			}
		}
	}
};

class GameState
{
public:
	Map map;

public:

	GameState()
		: map { }
	{
		//
	}

	void render()
	{
		map.render();
	}
};

class Game
{
	bool running = false;
	GameState currentState;
	std::shared_ptr<SDL_Window*> window = std::make_shared_for_overwrite<SDL_Window*>();
	std::shared_ptr<SDL_Surface*> surface = std::make_shared_for_overwrite<SDL_Surface*>();

public:
	Game()
		: currentState { }
	{
		if (SDL_Init(SDL_INIT_VIDEO) < 0) {
			std::cerr << "[Error] Could not initialize SDL: " << SDL_GetError() << "\n";
			std::exit(EXIT_FAILURE);
		}

		window = std::make_shared<SDL_Window*>(SDL_CreateWindow("Minesweeper", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 320, 32 + 320, SDL_WINDOW_SHOWN));
		surface = std::make_shared<SDL_Surface*>(SDL_GetWindowSurface(*window));
		
		// TODO: Refactor
		renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED);

		if (!renderer) {
			std::cerr << "[Error] Failed to initialize renderer: " << SDL_GetError() << "\n";
			std::exit(EXIT_FAILURE);
		}
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

				case SDL_MOUSEBUTTONUP: {
					int x_tile = (event.button.x) / Configuration::TileSize;
					int y_tile = (event.button.y - 32) / Configuration::TileSize;

					// TODO: Refactor please
					currentState.map.click(x_tile, y_tile);
				} break;

				case SDL_KEYUP: {
					switch (event.key.keysym.sym) {
						case SDLK_ESCAPE: {
							SDL_Event quit_event;
							quit_event.type = SDL_QUIT;
							SDL_PushEvent(&quit_event);
						} break;
					}
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
		SDL_Rect score_viewport = { 0, 0, 320, 32 };
		SDL_Rect map_viewport = { 0, 32, 320, 320 };

		// TODO: Refactor
		SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
		SDL_RenderClear(renderer);

		// Render the map.
		SDL_RenderSetViewport(renderer, &map_viewport);
		currentState.render();

		SDL_RenderPresent(renderer);

		// TODO: FPS correction to not abuse the CPU.
		SDL_Delay(16);
	}
};

int main(int argc, char *argv[])
{
	Game().start();
	return EXIT_SUCCESS;
}
