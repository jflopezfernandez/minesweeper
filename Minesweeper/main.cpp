
#include <cstdlib>

#include <iostream>
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

std::random_device random_device;
std::mt19937_64 generator(random_device());
std::binomial_distribution tile_distribution(1, 0.05);

TTF_Font* font = nullptr;

SDL_Renderer* renderer = nullptr;

SDL_Color blue = { 0x00, 0x00, 0xFF, 0xFF };
SDL_Color red = { 0xFF, 0x00, 0x00, 0xFF };

namespace Configuration
{
	size_t TileSize = 32;
} /** Namespace Configuration */

class IGameObject
{
public:
	virtual void render() = 0;
};

enum class GameEndResult
{
	Victory,
	Defeat
};

enum class TileClickResponse
{
	Nothing,
	Revealed,
	Exploded
};

enum class TileState
{
	Undrawn,
	Hidden,
	Revealed,
	Flagged,
	Exploded
};

class Tile : IGameObject
{
	int x;
	int y;

	bool is_bomb = false;
	size_t adjacent_bombs = 0;

	TileState state = TileState::Hidden;
	TileState previousState = TileState::Undrawn;

public:
	Tile(int x, int y)
		: x { x }, y { y }
	{
		//
	}

	int getX() const noexcept
	{
		return x;
	}

	int getY() const noexcept
	{
		return y;
	}

	void setBomb()
	{
		is_bomb = true;
		previousState = TileState::Undrawn;
	}

	bool isBomb() const noexcept
	{
		return is_bomb;
	}

	void setAdjacentBombs(size_t n)
	{
		adjacent_bombs = n;
	}

	size_t getAdjacentBombs() const noexcept
	{
		return adjacent_bombs;
	}

	bool isRevealable() const noexcept
	{
		return (state == TileState::Hidden) && (!isBomb());
	}

	void reveal() noexcept
	{
		state = TileState::Revealed;
	}

	/** TODO: This was the last thing we were working on. We need to reveal all adjacent tiles which have no bombs adjacent to themselves. */
	TileClickResponse click(bool isLeftClick)
	{
		// TODO: Implement check whether this tile has a bomb.
		if (isLeftClick) {
			switch (state) {
				case TileState::Hidden: {
					if (isBomb()) {
						// TODO: Lose the game
						state = TileState::Exploded;
						return TileClickResponse::Exploded;
					} else {
						//state = TileState::Revealed;
						return TileClickResponse::Revealed;
					}
				} break;
			}
		} else {
			if (state == TileState::Hidden) {
				state = TileState::Flagged;
			} else if (state == TileState::Flagged) {
				state = TileState::Hidden;
			}
		}

		return TileClickResponse::Nothing;
	}

	/**
	 * Hidden: Gray
	 * Revealed: Green
	 * Flagged: Yellow
	 * Exploded: Red
	 * 
	 */
	void render()
	{
		if (state != previousState) {
			switch (state) {
				case TileState::Hidden: {
					SDL_SetRenderDrawColor(renderer, 0xF0, 0xF0, 0xF0, 0xFF);

					// TODO: Remove
					if (isBomb()) {
						SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0xFF, 0xFF);
					}
				} break;

				case TileState::Revealed: {
					SDL_SetRenderDrawColor(renderer, 0x00, 0xFF, 0x00, 0xFF);
				} break;

				case TileState::Flagged: {
					SDL_SetRenderDrawColor(renderer, 0xFF, 0x00, 0xFF, 0xFF);
				} break;

				case TileState::Exploded: {
					SDL_SetRenderDrawColor(renderer, 0xFF, 0x00, 0x00, 0xFF);
				} break;
			}

			SDL_Rect rect = { x * Configuration::TileSize, y * Configuration::TileSize, Configuration::TileSize, Configuration::TileSize };
			SDL_RenderFillRect(renderer, &rect);

			/** Draw the outline of the tile */
			SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
			SDL_RenderDrawRect(renderer, &rect);

			if ((adjacent_bombs) && (state == TileState::Revealed)) {
				SDL_Color black = { 0x00, 0x00, 0x00, 0xFF };
				const auto surface = TTF_RenderText_Solid(font, std::to_string(adjacent_bombs).c_str(), black);
				const auto texture = SDL_CreateTextureFromSurface(renderer, surface);
				SDL_RenderCopy(renderer, texture, nullptr, &rect);
			}

			previousState = state;
		}
	}
};

class Map : IGameObject
{
	size_t tiles_wide;
	size_t tiles_high;
	size_t tile_size;

	std::vector<std::vector<Tile>> tiles;

	int nonBombTilesLeftToReveal = 0;

protected:
	void setBombs()
	{
		for (auto& row : tiles) {
			for (auto& tile : row) {
				if (tile_distribution(generator)) {
					tile.setBomb();
				} else {
					++nonBombTilesLeftToReveal;
				}
			}
		}
	}

	void calculateAdjacentBombs()
	{
		for (auto i = 0; i < tiles.size(); ++i) {
			for (auto j = 0; j < tiles[i].size(); ++j) {
				size_t nearbyBombs = 0;

				if (i > 0 && j > 0 && tiles[i - 1][j - 1].isBomb()) nearbyBombs++;
				if (i > 0 && tiles[i - 1][j].isBomb()) nearbyBombs++;
				if (i > 0 && (j < tiles[i].size() - 1) && tiles[i - 1][j + 1].isBomb()) nearbyBombs++;
				if (j > 0 && tiles[i][j - 1].isBomb()) nearbyBombs++;
				if ((j < tiles[i].size() - 1) && tiles[i][j + 1].isBomb()) nearbyBombs++;
				if ((i < tiles.size() - 1) && j > 0 && tiles[i + 1][j - 1].isBomb()) nearbyBombs++;
				if ((i < tiles.size() - 1) && tiles[i + 1][j].isBomb()) nearbyBombs++;
				if ((i < tiles.size() - 1) && (j < tiles[i].size() - 1) && tiles[i + 1][j + 1].isBomb()) nearbyBombs++;

				tiles[i][j].setAdjacentBombs(nearbyBombs);
			}
		}
	}

//protected:
	//bool revealTile(Tile& tile);

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

	void initialize(int x, int y)
	{
		setBombs();
		calculateAdjacentBombs();
	}

	bool click(int x, int y, bool isLeftClick)
	{
		const auto response = tiles[y][x].click(isLeftClick);
		auto game_over = false;

		// TODO: Actually use the click response.
		switch (response) {
			case TileClickResponse::Revealed: {
				game_over = revealTile(tiles[y][x]);
				const auto game_over_state = GameEndResult::Victory;
			} break;

			case TileClickResponse::Exploded: {
				// TODO: Game Over -> Prepare to reset
			} break;
		}

		return game_over;
	}

	void render()
	{
		for (auto& row : tiles) {
			for (auto& tile : row) {
				tile.render();
			}
		}
	}

protected:
	void revealTiles(std::vector<Tile> queue)
	{
		while (queue.size()) {
			auto current = queue.back();
			queue.pop_back();

			if (current.isRevealable()) {
				if (current.getAdjacentBombs() == 0) {
					int i = current.getX();
					int j = current.getY();
					if (i > 0 && j > 0 && (tiles[i - 1][j - 1].isRevealable()))    queue.push_back(tiles[i - 1][j - 1]);
					if (i > 0 && (tiles[i - 1][j].isRevealable()))        queue.push_back(tiles[i - 1][j]);
					if (i > 0 && (j < tiles[i].size() - 1) && (tiles[i - 1][j + 1].isRevealable()))    queue.push_back(tiles[i - 1][j + 1]);
					if (j > 0 && (tiles[i][j - 1].isRevealable()))        queue.push_back(tiles[i][j - 1]);;
					if ((j < tiles[i].size() - 1) && (tiles[i][j + 1].isRevealable()))        queue.push_back(tiles[i][j + 1]);
					if ((i < tiles.size() - 1) && j > 0 && (tiles[i + 1][j - 1].isRevealable()))    queue.push_back(tiles[i + 1][j - 1]);
					if ((i < tiles.size() - 1) && (tiles[i + 1][j].isRevealable()))        queue.push_back(tiles[i + 1][j]);
					if ((i < tiles.size() - 1) && (j < tiles[i].size() - 1) && (tiles[i + 1][j + 1].isRevealable()))    queue.push_back(tiles[i + 1][j + 1]);
				}
				current.reveal();
			}
		}
	}

	bool revealTile(Tile& tile)
	{
		std::vector<Tile> tilesToReveal;
		tilesToReveal.push_back(tile);
		revealTiles(tilesToReveal);
		return nonBombTilesLeftToReveal == 0;
	}
};

class GameState
{
	bool initialized = false;
	Map map;

public:

	GameState()
		: map { }
	{
		//
	}

	bool click(int x, int y, bool isLeftClick)
	{
		if (!initialized && isLeftClick) {
			map.initialize(x, y);
			initialized = true;
		}

		return map.click(x, y, isLeftClick);
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

		if (TTF_Init() == -1) {
			SDL_LogError(SDL_LOG_CATEGORY_SYSTEM, "[Error] Could not initialize SDL Font library: %s\n", TTF_GetError());
			std::exit(EXIT_FAILURE);
		}

		window = std::make_shared<SDL_Window*>(SDL_CreateWindow("Minesweeper", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 20 * Configuration::TileSize, (2 * Configuration::TileSize) + (20 * Configuration::TileSize), SDL_WINDOW_SHOWN));
		surface = std::make_shared<SDL_Surface*>(SDL_GetWindowSurface(*window));
		
		// TODO: Refactor
		renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED);

		if (!renderer) {
			std::cerr << "[Error] Failed to initialize renderer: " << SDL_GetError() << "\n";
			std::exit(EXIT_FAILURE);
		}

		//font = TTF_OpenFont("m42.ttf", 8);
		//font = TTF_OpenFont("ShareTechMono-Regular.ttf", 10);
		font = TTF_OpenFont("C:\\Users\\jflop\\source\\repos\\Minesweeper\\x64\\Debug\\ShareTechMono-Regular.ttf", 10);

		if (!font) {
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", TTF_GetError(), *window);
			//std::exit(EXIT_FAILURE);
		}
	}

	~Game()
	{
		TTF_Quit();
		SDL_DestroyRenderer(renderer);
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
					int y_tile = (event.button.y - (2 * Configuration::TileSize)) / Configuration::TileSize;

					// TODO: Refactor please
					if ((x_tile >= 0) && (y_tile >= 0)) {
						bool left_click = event.button.button == SDL_BUTTON_LEFT;
						const auto state = currentState.click(x_tile, y_tile, left_click);

						if (state == true) {
							// TODO: Game over
							// ...
						}
					}
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
		SDL_Rect score_viewport = { 0, 0, (20 * Configuration:: TileSize), (2 * Configuration::TileSize) };
		SDL_Rect map_viewport = { 0, (2 * Configuration::TileSize), (20 * Configuration::TileSize), (20 * Configuration::TileSize) };

		// TODO: Refactor
		SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
		//SDL_RenderClear(renderer);

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
