#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <print>
#include <random>
#include <stdexcept>
#include <string>
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
  void draw_snake(WINDOW*, const std::vector<point_t>&);
  void draw_food(WINDOW*, point_t);
  void draw_frame(WINDOW*, const game_model&);
  void draw_gameover(WINDOW*, const game_model&);
  void print_center(WINDOW*, std::string_view, int);
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

void snake::draw_snake(WINDOW* win, const std::vector<point_t>& points) {
  const static auto printer = [win](auto p) { mvwprintw(win, p.y, p.x * 2, "%s", "[]"); };
  wattron(win, COLOR_PAIR(ncurses_guard::col_yellow));
  printer(points.front());
  wattroff(win, COLOR_PAIR(ncurses_guard::col_yellow));
  wattron(win, COLOR_PAIR(ncurses_guard::col_green));
  std::for_each(points.begin() + 1, points.end(), printer);
  wattroff(win, COLOR_PAIR(ncurses_guard::col_green));
}

void snake::draw_food(WINDOW* win, const point_t food) {
  wattron(win, COLOR_PAIR(ncurses_guard::col_red));
  mvwprintw(win, food.y, food.x * 2, "%s", "[]");
  wattroff(win, COLOR_PAIR(ncurses_guard::col_red));
}

void snake::draw_frame(WINDOW* win, const game_model& model) {
  werase(win);
  box(win, 0, 0);
  draw_snake(win, model.get_snake());
  draw_food(win, model.get_food());
  wrefresh(win);
}

void snake::draw_gameover(WINDOW* win, const game_model& model) {
  const auto scores_msg = "Scores: " + std::to_string(model.get_scores());
  const auto mid_y = getmaxy(win) / 2;
  print_center(win, "Game Over", mid_y - 1);
  print_center(win, scores_msg, mid_y);
  print_center(win, "Press q to exit", mid_y + 1);
  wrefresh(win);
}

void snake::print_center(WINDOW* win, const std::string_view str, const int y) {
  const auto x = (getmaxx(win) - str.size()) / 2;
  mvwaddnstr(win, y, x, str.data(), str.size());
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
  const auto height = std::min(LINES, 22);
  const auto width = std::min(COLS, 42);
  const auto win = newwin(height, width, 0, 0);
  auto model = snake::game_model(height, width / 2);
  auto is_running = true;
  while (model.step() && is_running) {
    snake::draw_frame(win, model);
    is_running = snake::parse_input(model) != snake::input_result::exit;
  }
  snake::draw_gameover(win, model);
  auto ch = getch();
  while (ch != 'q' && ch != 'Q') {
    ch = getch();
  }
} catch (const std::exception& e) {
  std::println(stderr, "Error: {}", e.what());
  return 1;
}
