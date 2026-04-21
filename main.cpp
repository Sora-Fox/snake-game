#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <print>
#include <random>
#include <stdexcept>
#include <vector>
#include <ncurses.h>
#include "game_model.hpp"

namespace snake {
  struct ncurses_guard;
  enum class input_result : std::uint8_t {
    success,
    exit,
  };
  [[nodiscard]] input_result parse_input(game_model&);
  void draw_snake(const std::vector<point_t>&);
  void draw_food(point_t);
  void draw_frame(const game_model&);
}

struct snake::ncurses_guard {
  static constexpr auto col_yellow = 1;
  static constexpr auto col_green = 2;
  static constexpr auto col_red = 3;

  ncurses_guard();
  ~ncurses_guard();
};

snake::ncurses_guard::ncurses_guard() {
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, true);
  curs_set(0);
  timeout(150);
  if (!has_colors()) {
    endwin();
    throw std::runtime_error("Colors are not supported");
  }
  start_color();
  init_pair(col_yellow, COLOR_YELLOW, COLOR_BLACK);
  init_pair(col_green, COLOR_GREEN, COLOR_BLACK);
  init_pair(col_red, COLOR_RED, COLOR_BLACK);
}

snake::ncurses_guard::~ncurses_guard() {
  endwin();
}

void snake::draw_snake(const std::vector<point_t>& points) {
  const static auto printer = [](auto p) { mvprintw(p.y, p.x * 2, "%s", "[]"); };
  attron(COLOR_PAIR(ncurses_guard::col_yellow));
  printer(points.front());
  attroff(COLOR_PAIR(ncurses_guard::col_yellow));
  attron(COLOR_PAIR(ncurses_guard::col_green));
  std::for_each(points.begin() + 1, points.end(), printer);
  attroff(COLOR_PAIR(ncurses_guard::col_green));
}

void snake::draw_food(const point_t food) {
  attron(COLOR_PAIR(ncurses_guard::col_red));
  mvprintw(food.y, food.x * 2, "%s", "[]");
  attroff(COLOR_PAIR(ncurses_guard::col_red));
}

void snake::draw_frame(const game_model& model) {
  erase();
  draw_snake(model.get_snake());
  draw_food(model.get_food());
  refresh();
}

snake::input_result snake::parse_input(game_model& model) {
  switch (getch()) {
  case KEY_UP:
  case 'k':
    model.set_direction(snake::direction::up);
    break;
  case KEY_DOWN:
  case 'j':
    model.set_direction(snake::direction::down);
    break;
  case KEY_RIGHT:
  case 'l':
    model.set_direction(snake::direction::right);
    break;
  case KEY_LEFT:
  case 'h':
    model.set_direction(snake::direction::left);
    break;
  case 'q':
    return input_result::exit;
  }
  return input_result::success;
}

int main() try {
  const auto guard = snake::ncurses_guard{};
  snake::game_model model(LINES, COLS / 2);
  bool is_running = true;
  while (model.step() && is_running) {
    snake::draw_frame(model);
    is_running = snake::parse_input(model) != snake::input_result::exit;
  }
} catch (const std::exception& e) {
  std::println(stderr, "Error: {}", e.what());
  return 1;
}
