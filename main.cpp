#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <random>
#include <vector>
#include <ncurses.h>
#include "game_model.hpp"

int main() {
  initscr();
  cbreak();
  noecho();
  clear();
  keypad(stdscr, TRUE);
  curs_set(0);
  timeout(150);
  if (has_colors() == FALSE) {
    endwin();
    printf("Your terminal does not support color\n");
    exit(1);
  }
  start_color(); /* Start color 			*/
  int cols, rows;
  getmaxyx(stdscr, rows, cols);
  snake::game_model game(rows, cols / 2);
  while (game.step()) {
    clear();
    auto points = game.get_snake();
    auto head = points.front();
    init_pair(1, COLOR_YELLOW, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_RED, COLOR_BLACK);
    attron(COLOR_PAIR(1));
    mvprintw(points.front().y, points.front().x * 2, "%s", "[]");
    attroff(COLOR_PAIR(1));
    // if (head.y == points[2].y) {
    //    mvprintw(points.front().y, points.front().x * 2, "%s", "xx");
    //    } else {
    //      mvprintw(points.front().y, points.front().x * 2, "%s", "**");
    //    }
    attron(COLOR_PAIR(2));
    for (auto i = points.begin() + 1; i != points.end(); ++i) {
      auto p = *i;
      mvprintw(p.y, p.x * 2, "%s", "[]");
    }
    attroff(COLOR_PAIR(2));
    auto f = game.get_food();
    attron(COLOR_PAIR(3));
    mvprintw(f.y, f.x * 2, "%s", "[]");
    attroff(COLOR_PAIR(3));
    int c = getch();
    switch (c) {
    case KEY_UP:
    case 'k':
      game.set_direction(snake::direction::up);
      break;
    case KEY_DOWN:
    case 'j':
      game.set_direction(snake::direction::down);
      break;
    case KEY_RIGHT:
    case 'l':
      game.set_direction(snake::direction::right);
      break;
    case KEY_LEFT:
    case 'h':
      game.set_direction(snake::direction::left);
      break;
    case 'q':
      endwin();
      return 0;
    }
    refresh();
  }
  endwin();
}
