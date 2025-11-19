#include <SFML/Graphics.hpp>
#include <filesystem>
#include <random>
#include <string>
#include <deque>

// for visual studio, hide the console.
#pragma comment( linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"" )

namespace fs = std::filesystem;

const int GRID_SIZE = 20;
const int GRID_WIDTH = 40;
const int GRID_HEIGHT = 30;
const int WINDOW_HEIGHT = GRID_SIZE * GRID_HEIGHT;
const int WINDOW_WIDTH = GRID_SIZE * GRID_WIDTH;
const int FPS = 10;

enum class Direction {
    up,
    down,
    left,
    right
};

class Snake {
    std::deque<sf::Vector2i> body;
    Direction direction;
    int growPending;
public:
    Snake()
        : body{}, direction{ Direction::left }, growPending{ 3 }
    {
        body.push_back(sf::Vector2i{ GRID_WIDTH / 2, GRID_HEIGHT / 2 });
    }

    void set_direction(Direction d) noexcept {
        // reverse direction is not allowed.
        if (direction == Direction::up && d == Direction::down) {
            return;
        }

        if (direction == Direction::down && d == Direction::up) {
            return;
        }

        if (direction == Direction::left && d == Direction::right) {
            return;
        }

        if (direction == Direction::right && d == Direction::left) {
            return;
        }

        direction = d;
    }

    bool update() {
        sf::Vector2i newHead = body.front();

        switch (direction) {
        case Direction::up:
            --newHead.y;
            break;
        case Direction::down:
            ++newHead.y;
            break;
        case Direction::left:
            --newHead.x;
            break;
        case Direction::right:
            ++newHead.x;
            break;
        }

        // enable cross the boundary.
        /*if (newHead.x < 0) {
            newHead.x = GRID_WIDTH - 1;
        }

        if (newHead.x >= GRID_WIDTH) {
            newHead.x = 0;
        }

        if (newHead.y < 0) {
            newHead.y = GRID_HEIGHT - 1;
        }

        if (newHead.y >= GRID_HEIGHT) {
            newHead.y = 0;
        }*/

        if (newHead.x < 0 || newHead.y < 0 || newHead.x >= GRID_WIDTH || newHead.y >= GRID_HEIGHT) {
            return false;
        }

        body.push_front(newHead);

        if (growPending > 0) {
            --growPending;
        }
        else {
            body.pop_back();
        }

        return true;
    }

    void body_grow() {
        ++growPending;
    }

    bool check_self_collision() {
        const auto& head = get_head();

        // compare the head with the remaining ones.
        for (int i = 1; i < body.size(); ++i) {
            if (head == body[i]) {
                return true;
            }
        }

        return false;
    }

    void reset() {
        body.clear();
        body.push_back(sf::Vector2i{ GRID_WIDTH / 2, GRID_HEIGHT / 2 });

        growPending = 3;
        direction = Direction::left;
    }

    const sf::Vector2i& get_head() const noexcept {
        return body.front();
    }

    const std::deque<sf::Vector2i>& get_body() const noexcept {
        return body;
    }
};

class Food {
    std::random_device rd;
    std::mt19937_64 gen;
    std::uniform_int_distribution<int> distX;
    std::uniform_int_distribution<int> distY;
    sf::Vector2i position;
public:
    Food()
        : rd{}, gen{ rd() }, distX{ 0, GRID_WIDTH - 1 }, distY{ 0, GRID_HEIGHT - 1 }
    {
        respawn();
    }

    void respawn() {
        position.x = distX(gen);
        position.y = distY(gen);
    }

    const sf::Vector2i& get_position() const noexcept {
        return position;
    }
};

class Game {
    sf::RenderWindow window;
    sf::Font font;
    sf::Text scoreText;
    sf::Text gameOverText;
    Snake snake;
    Food food;
    int score;
    bool gameOver;

    void handle_events() {
        sf::Event event;

        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }

            if (event.type == sf::Event::KeyPressed) {
                // restart game.
                if (gameOver && event.key.code == sf::Keyboard::R) {
                    snake.reset();
                    food.respawn();
                    score = 0;
                    gameOver = false;
                }
                else if (!gameOver) {
                    // handle direciton keys.
                    switch (event.key.code) {
                    case sf::Keyboard::Up:
                        snake.set_direction(Direction::up);
                        return;
                    case sf::Keyboard::Down:
                        snake.set_direction(Direction::down);
                        return;
                    case sf::Keyboard::Left:
                        snake.set_direction(Direction::left);
                        return;
                    case sf::Keyboard::Right:
                        snake.set_direction(Direction::right);
                        return;
                    }
                }
            }
        }
    }

    void update() {
        if (!snake.update()) {
            gameOver = true;
            return;
        }

        if (snake.get_head() == food.get_position()) {
            snake.body_grow();
            food.respawn();
            score += 10;
        }

        if (snake.check_self_collision()) {
            gameOver = true;
        }

        scoreText.setString("Score: " + std::to_string(score));
    }

    void render() {
        window.clear(sf::Color::Black);

        // render snake.
        for (const sf::Vector2i& part : snake.get_body()) {
            sf::RectangleShape rect{ sf::Vector2f{ GRID_SIZE, GRID_SIZE } };

            rect.setPosition(part.x * GRID_SIZE, part.y * GRID_SIZE);
            rect.setFillColor(sf::Color::Green);
            window.draw(rect);
        }

        // render food.
        const sf::Vector2i& foodPos = food.get_position();
        sf::RectangleShape foodRect{ sf::Vector2f{ GRID_SIZE, GRID_SIZE } };
        foodRect.setPosition(foodPos.x * GRID_SIZE, foodPos.y * GRID_SIZE);
        foodRect.setFillColor(sf::Color::Red);
        window.draw(foodRect);

        // render score.
        window.draw(scoreText);

        // game over.
        if (gameOver) {
            window.draw(gameOverText);
        }

        window.display();
    }
public:
    Game()
        : window{ sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Snake" },
        snake{},
        food{},
        score{ 0 },
        gameOver{ false }
    {
        window.setFramerateLimit(FPS);

        font.loadFromFile("arial.ttf");

        scoreText.setFont(font);
        scoreText.setCharacterSize(24);
        scoreText.setFillColor(sf::Color::White);
        scoreText.setPosition(10, 10);

        gameOverText.setFont(font);
        gameOverText.setCharacterSize(48);
        gameOverText.setFillColor(sf::Color::White);
        gameOverText.setString("Game Over!\nPress R to restart");
        gameOverText.setPosition(WINDOW_WIDTH / 2 - 150, WINDOW_HEIGHT / 2 - 50);
    }

    void run() {
        while (window.isOpen()) {
            handle_events();

            if (!gameOver) {
                update();
            }

            render();
        }
    }
};

int main() {
    if (!fs::exists("arial.ttf")) {
        return 1;
    }

    Game game;
    game.run();
    return 0;
}
