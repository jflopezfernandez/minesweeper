
#include <cstdlib>

#include <chrono>
#include <iostream>
#include <map>
#include <memory>
#include <queue>
#include <random>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

namespace Configuration
{
	namespace Map
	{
		namespace Size
		{
			constexpr size_t Width = 20;
			constexpr size_t Height = 20;
		} /** Namespace Map */

	} /** Namespace Map */

	size_t TileSize = 32;
} /** Namespace Configuration */

std::random_device random_device;
std::seed_seq random_seed { 0 };
std::mt19937_64 generator(random_seed);
std::binomial_distribution tile_distribution(1, 0.065);

TTF_Font* font = nullptr;
TTF_Font* score_font = nullptr;

SDL_Renderer* renderer = nullptr;

SDL_Color blue = { 0x00, 0x00, 0xFF, 0xFF };
SDL_Color red = { 0xFF, 0x00, 0x00, 0xFF };

bool game_active = true;

class IGameObject
{
public:
	virtual void update() = 0;
	virtual void render() = 0;
};

class Scoreboard : IGameObject
{
	std::chrono::time_point<std::chrono::high_resolution_clock> time_started = std::chrono::high_resolution_clock::now();
	std::ostringstream output;

public:
	Scoreboard()
	{
		/** Default constructor */
	}

	virtual void update()
	{
		const auto current_time = std::chrono::high_resolution_clock::now();
		const auto duration = std::chrono::duration_cast<std::chrono::seconds>(current_time - time_started) * 360;
		
		/** Reset the output buffer */
		output = std::ostringstream { };
		output << duration;

		// TODO: Remove after testing.
		// std::clog << output.str() << "\n";
	}

	virtual void render()
	{
		SDL_Rect rect = { 5, 0, Configuration::TileSize * 4, Configuration::TileSize * 2 };
		SDL_Color black = { 0x00, 0x00, 0x00, 0xFF };
		SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
		SDL_RenderFillRect(renderer, &rect);

		SDL_Color white = { 0xFF, 0xFF, 0xFF, 0xFF };
		const auto surface = TTF_RenderText_Solid(score_font, output.str().c_str(), white);
		const auto texture = SDL_CreateTextureFromSurface(renderer, surface);
		SDL_RenderCopy(renderer, texture, nullptr, &rect);
	}
};

enum class GameStatus
{
	Active,
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

	std::vector<std::shared_ptr<Tile>> adjacent_tiles { };

public:
	Tile(int x, int y) noexcept
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

	void setBomb() noexcept
	{
		is_bomb = true;
		previousState = TileState::Undrawn;
	}

	bool isBomb() const noexcept
	{
		return is_bomb;
	}

	void setAdjacentBombs(size_t n) noexcept
	{
		adjacent_bombs = n;
	}

	size_t getAdjacentBombs() const noexcept
	{
		return adjacent_bombs;
	}

	bool removeBomb() noexcept
	{
		if (isBomb()) {
			is_bomb = false;
			return true;
		}

		return false;
	}

	void addAdjacentTile(std::shared_ptr<Tile>& tile) noexcept
	{
		adjacent_tiles.emplace_back(tile);
	}

	bool isRevealable() const noexcept
	{
		return (state == TileState::Hidden) && (!isBomb());
	}

	std::vector<std::shared_ptr<Tile>>& getAdjacentTiles() noexcept
	{
		return adjacent_tiles;
	}

	bool reveal() noexcept
	{
		if (state != TileState::Revealed) {
			state = TileState::Revealed;
			return true;
		}

		return false;
	}

	TileClickResponse click(bool isLeftClick) noexcept
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

	virtual void update()
	{
		//
	}

	/**
	 * Hidden: Gray
	 * Revealed: Green
	 * Flagged: Yellow
	 * Exploded: Red
	 * 
	 */
	virtual void render()
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

void addTileToQueue(std::queue<std::shared_ptr<Tile>>& queue, std::map<std::pair<int, int>, bool>& tilesMap, std::shared_ptr<Tile> tile) noexcept
{
	/*for (const auto& t : queue) {
		if (t.getX() == tile.getX() && t.getY() == tile.getY()) {
			return;
		}
	}*/

	if (tilesMap.find(std::make_pair<int, int>(tile->getX(), tile->getY())) != tilesMap.end()) {
		return;
	}

	queue.push(tile);
}

class Map : IGameObject
{
	size_t tiles_wide;
	size_t tiles_high;
	size_t tile_size;

	std::vector<std::vector<std::shared_ptr<Tile>>> tiles { };

	int nonBombTilesLeftToReveal = 0;

protected:
	void setBombs()
	{
		for (auto& row : tiles) {
			for (auto& tile : row) {
				if (tile_distribution(generator)) {
				//if ((tile->getX() == 0 && tile->getY() == 0) || (tile->getX() == 0 && tile->getY() == 2)) {
					tile->setBomb();
				} else {
					++nonBombTilesLeftToReveal;
				}
			}
		}
	}

	void calculateAdjacentBombs() noexcept
	{
		for (auto i = 0; i < tiles.size(); ++i) {
			for (auto j = 0; j < tiles[i].size(); ++j) {
				size_t nearbyBombs = 0;

				if (i > 0 && j > 0 && tiles[i - 1][j - 1]->isBomb()) nearbyBombs++;
				if (i > 0 && tiles[i - 1][j]->isBomb()) nearbyBombs++;
				if (i > 0 && (j < tiles[i].size() - 1) && tiles[i - 1][j + 1]->isBomb()) nearbyBombs++;
				if (j > 0 && tiles[i][j - 1]->isBomb()) nearbyBombs++;
				if ((j < tiles[i].size() - 1) && tiles[i][j + 1]->isBomb()) nearbyBombs++;
				if ((i < tiles.size() - 1) && j > 0 && tiles[i + 1][j - 1]->isBomb()) nearbyBombs++;
				if ((i < tiles.size() - 1) && tiles[i + 1][j]->isBomb()) nearbyBombs++;
				if ((i < tiles.size() - 1) && (j < tiles[i].size() - 1) && tiles[i + 1][j + 1]->isBomb()) nearbyBombs++;

				tiles[i][j]->setAdjacentBombs(nearbyBombs);
			}
		}
	}

public:
	Map(size_t tiles_wide = Configuration::Map::Size::Width, size_t tiles_high = Configuration::Map::Size::Height, size_t tile_size = Configuration::TileSize)
		: tiles_wide { tiles_wide }, tiles_high { tiles_high }, tile_size { tile_size }
	{
		/** Create the map by populating it with tiles. */
		for (auto h = 0; h < tiles_high; ++h) {
			std::vector<std::shared_ptr<Tile>> row { };

			for (auto w = 0; w < tiles_wide; ++w) {
				row.emplace_back(std::make_shared<Tile>(Tile(w, h)));
			}

			tiles.push_back(row);
		}

		/** Initialize each tile's adjacent tiles list. */
		for (auto i = 0; i < tiles.size(); ++i) {
			for (auto j = 0; j < tiles[i].size(); ++j) {
				if (i > 0 && j > 0) tiles[i][j]->addAdjacentTile(tiles[i-1][j-1]);
				if (i > 0) tiles[i][j]->addAdjacentTile(tiles[i-1][j]);
				if (i > 0 && (j < tiles[i].size() - 1)) tiles[i][j]->addAdjacentTile(tiles[i-1][j+1]);
				if (j > 0) tiles[i][j]->addAdjacentTile(tiles[i][j-1]);
				if ((j < tiles[i].size() - 1)) tiles[i][j]->addAdjacentTile(tiles[i][j+1]);
				if ((i < tiles.size() - 1) && j > 0) tiles[i][j]->addAdjacentTile(tiles[i+1][j-1]);
				if ((i < tiles.size() - 1)) tiles[i][j]->addAdjacentTile(tiles[i+1][j]);
				if ((i < tiles.size() - 1) && (j < tiles[i].size() - 1)) tiles[i][j]->addAdjacentTile(tiles[i+1][j+1]);
			}
		}
	}

	void initialize(int x, int y) noexcept
	{
		/**
		 * TODO: Use the x and y coordinates to make sure
		 * there are no bombs placed in the current tile and
		 * in the adjacent ones.
		 */
		setBombs();

		if (tiles[y][x]->removeBomb()) {
			++nonBombTilesLeftToReveal;
		}

		for (auto& tile : tiles[y][x]->getAdjacentTiles()) {
			if (tile->removeBomb()) {
				++nonBombTilesLeftToReveal;
			}
		}

		calculateAdjacentBombs();
	}

	GameStatus click(int x, int y, bool isLeftClick)
	{
		const auto response = tiles[y][x]->click(isLeftClick);

		switch (response) {
			case TileClickResponse::Revealed: {
				/**
				 * The revealTile function returns true if there are
				 * no more non-bomb tiles to reveal.
				 * 
				 */
				if (revealTile(tiles[y][x])) {
					return GameStatus::Victory;
				}
			} break;

			case TileClickResponse::Exploded: {
				return GameStatus::Defeat;
			} break;
		}

		return GameStatus::Active;
	}

	virtual void update()
	{
		//
	}

	virtual void render()
	{
		for (auto& row : tiles) {
			for (auto& tile : row) {
				tile->render();
			}
		}
	}

protected:
	void revealTiles(std::queue<std::shared_ptr<Tile>>& queue, std::map<std::pair<int, int>, bool>& tilesTable)
	{
		while (queue.size()) {
			auto current = queue.front();
			queue.pop();

			int i = current->getY();
			int j = current->getX();

			if (tiles[i][j]->isRevealable()) {
				if (tiles[i][j]->getAdjacentBombs() == 0) {
					if (i > 0 && j > 0 && (tiles[i - 1][j - 1]->isRevealable()))    addTileToQueue(queue, tilesTable, tiles[i - 1][j - 1]);
					if (i > 0 && (tiles[i - 1][j]->isRevealable()))        addTileToQueue(queue, tilesTable, tiles[i - 1][j]);
					if (i > 0 && (j < tiles[i].size() - 1) && (tiles[i - 1][j + 1]->isRevealable()))    addTileToQueue(queue, tilesTable, tiles[i - 1][j + 1]);
					if (j > 0 && (tiles[i][j - 1]->isRevealable()))        addTileToQueue(queue, tilesTable, tiles[i][j - 1]);;
					if ((j < tiles[i].size() - 1) && (tiles[i][j + 1]->isRevealable()))        addTileToQueue(queue, tilesTable, tiles[i][j + 1]);
					if ((i < tiles.size() - 1) && j > 0 && (tiles[i + 1][j - 1]->isRevealable()))    addTileToQueue(queue, tilesTable, tiles[i + 1][j - 1]);
					if ((i < tiles.size() - 1) && (tiles[i + 1][j]->isRevealable()))        addTileToQueue(queue, tilesTable, tiles[i + 1][j]);
					if ((i < tiles.size() - 1) && (j < tiles[i].size() - 1) && (tiles[i + 1][j + 1]->isRevealable()))    addTileToQueue(queue, tilesTable, tiles[i + 1][j + 1]);
				}
				
				//if (current.reveal()) {
				if (tiles[i][j]->reveal()) {
					--nonBombTilesLeftToReveal;
				}

				tiles[i][j] = current;
			}
		}
	}

	bool revealTile(std::shared_ptr<Tile>& tile)
	{
		std::map<std::pair<int, int>, bool> tilesTable;
		const auto tile_coordinates = std::make_pair(tile->getX(), tile->getY());
		tilesTable.insert({ tile_coordinates, true });

		std::queue<std::shared_ptr<Tile>> tilesToReveal;
		tilesToReveal.push(tile);
		revealTiles(tilesToReveal, tilesTable);
		return nonBombTilesLeftToReveal == 0;
	}
};

class SessionState
{
	bool initialized = false;
	Map map;
	Scoreboard scoreboard;

public:
	SessionState()
		: map { }, scoreboard { }
	{
		//
	}

	GameStatus click(int x, int y, bool isLeftClick)
	{
		if (!initialized && isLeftClick) {
			map.initialize(x, y);
			initialized = true;
		}

		return map.click(x, y, isLeftClick);
	}

	virtual void update()
	{
		scoreboard.update();
	}

	virtual void render()
	{
		/** Render the map. */
		SDL_Rect map_viewport = { 0, (2 * Configuration::TileSize), (20 * Configuration::TileSize), (20 * Configuration::TileSize) };
		SDL_RenderSetViewport(renderer, &map_viewport);
		map.render();

		/** Render the scoreboard. */
		SDL_Rect score_viewport = { 0, 0, (20 * Configuration::TileSize), (2 * Configuration::TileSize) };
		SDL_RenderSetViewport(renderer, &score_viewport);
		scoreboard.render();
	}
};

class Game
{
	bool running = false;
	bool message_shown = false;
	SessionState currentState;
	GameStatus gameStatus = GameStatus::Active;
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
			std::exit(EXIT_FAILURE);
		}

		score_font = TTF_OpenFont("C:\\Users\\jflop\\source\\repos\\Minesweeper\\x64\\Debug\\RobotoMono-Light.ttf", 72);

		if (!score_font) {
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", TTF_GetError(), *window);
			std::exit(EXIT_FAILURE);
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
		if (gameStatus != GameStatus::Active) {
			if (!SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Minesweeper", ((gameStatus == GameStatus::Victory) ? "Victory" : "Defeat"), *window)) {
				reset();
			}
		}

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
						gameStatus = currentState.click(x_tile, y_tile, left_click);
					} else {
						// TODO: Viewport clicks
					}
				} break;

				case SDL_KEYUP: {
					switch (event.key.keysym.sym) {
						case SDLK_ESCAPE: {
							SDL_Event quit_event;
							quit_event.type = SDL_QUIT;
							SDL_PushEvent(&quit_event);
						} break;

						case SDLK_RETURN: {
							currentState = SessionState { };
						} break;

						case SDLK_SPACE: {
							SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Minesweeper", "The game is paused.", *window);
						} break;
					}
				} break;
			}
		}
	}

	void update()
	{
		currentState.update();
	}

	void render()
	{
		// TODO: Refactor
		SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
		//SDL_RenderClear(renderer);

		currentState.render();

		SDL_RenderPresent(renderer);

		// TODO: FPS correction to not abuse the CPU.
		SDL_Delay(16);
	}

	void reset() noexcept
	{
		currentState = SessionState { };
		gameStatus = GameStatus::Active;
	}
};

int main(int argc, char *argv[])
{
	Game().start();
	return EXIT_SUCCESS;
}
