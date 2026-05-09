#include <chrono>
#include <cstdio>
#include <print>
#include <stdexcept>
#include <thread>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>

#include "game_model.hpp"

namespace snake {
  struct game_theme;
  class game_view;
  class x11_view;
}

struct snake::game_theme {
  const char* name;
  unsigned long border_color;
  unsigned long guidance_color;
  unsigned long snake_head_color;
  unsigned long snake_body_color;
  unsigned long background_color;
  unsigned long food_color;
  unsigned long grid_color;
  bool show_grid;
  bool show_guidance;
};

class snake::game_view {
public:
  enum class command : std::uint8_t { none, up, down, left, right, exit, switch_theme };

  virtual ~game_view() = 0;

  virtual bool open() noexcept = 0;
  [[nodiscard]] virtual bool is_open() const noexcept = 0;

  virtual void close() noexcept = 0;
  [[nodiscard]] virtual bool should_close() const noexcept = 0;

  virtual void render(const game_model&, const game_theme&) = 0;
  [[nodiscard]] virtual command poll_input() = 0;
};

snake::game_view::~game_view() {};

class snake::x11_view final : public game_view {
public:
  x11_view() noexcept = default;
  ~x11_view() override;

  bool open() noexcept override;
  bool is_open() const noexcept override;

  void close() noexcept override;
  bool should_close() const noexcept override;

  void render(const game_model&, const game_theme&) override;
  command poll_input() override;

private:
  Display* display_ = nullptr;
  Screen* screen_ = nullptr;
  Window win_{};
  GC gc_{};
  Atom wm_delete_{};
  bool should_close_ = false;
  bool is_open_ = false;
};

bool snake::x11_view::open() noexcept {
  display_ = XOpenDisplay(nullptr);
  if (!display_) {
    std::println(stderr, "Failed to open display");
    return false;
  }

  const auto screen_number = XDefaultScreen(display_);
  screen_ = XScreenOfDisplay(display_, screen_number);
  if (!screen_) {
    std::println(stderr, "Failed to get screen");
    XCloseDisplay(display_);
    return false;
  }
  const auto black = XBlackPixelOfScreen(screen_);
  const auto root = XRootWindowOfScreen(screen_);
  constexpr static auto h = 832;
  constexpr static auto w = 832;
  win_ = XCreateSimpleWindow(display_, root, 0, 0, w, h, 0, black, black);
  gc_ = XCreateGC(display_, win_, 0, nullptr);
  wm_delete_ = XInternAtom(display_, "WM_DELETE_WINDOW", false);
  XSelectInput(display_, win_, ExposureMask | KeyPressMask);
  XSetWMProtocols(display_, win_, &wm_delete_, 1);
  XMapWindow(display_, win_);
  XStoreName(display_, win_, "Snake Game");
  std::println(stderr, "Vendor  {}", XServerVendor(display_));
  std::println(stderr, "Release {}", XVendorRelease(display_));
  std::println(stderr, "Display {}", XDisplayString(display_));
  is_open_ = true;
  XEvent e;
  while (true) {
    XNextEvent(display_, &e);
    if (e.type == Expose) {
      break;
    }
  }
  return true;
};

snake::x11_view::~x11_view() {
  close();
}

void snake::x11_view::close() noexcept {
  if (!is_open()) {
    return;
  }
  XFreeGC(display_, gc_);
  XDestroyWindow(display_, win_);
  XCloseDisplay(display_);
  std::println(stderr, "X connection was closed");
  is_open_ = false;
  should_close_ = false;
}

void snake::x11_view::render(const game_model& model, const game_theme& theme) {
  // draw_background
  // draw_guidance
  // draw_food
  // draw_snake
  // draw_grid
  // draw_border

  auto [h, w] = std::pair(model.rows(), model.cols());
  auto attrs = XWindowAttributes{};
  XGetWindowAttributes(display_, win_, &attrs);
  auto tile_size = std::min(attrs.width / (w + 1), attrs.height / (h + 1));
  auto x_offset = (attrs.width - 1 - tile_size * w) / 2;
  auto y_offset = (attrs.height - 1 - tile_size * h) / 2;

  auto draw_tile = [x_offset, y_offset, tile_size, theme, this](int x, int y,
                       unsigned long color) {
    const auto x_coord = x * tile_size + x_offset;
    const auto y_coord = y * tile_size + y_offset;
    XSetForeground(display_, gc_, color);
    XFillRectangle(display_, win_, gc_, x_coord, y_coord, tile_size, tile_size);
    XSetForeground(display_, gc_, theme.grid_color);
    XDrawRectangle(display_, win_, gc_, x_coord, y_coord, tile_size, tile_size);
  };

  XSetBackground(display_, gc_, theme.background_color);
  XClearWindow(display_, win_);

  const auto food = model.get_food();
  for (size_t x = 0; x != model.cols(); ++x) {
    draw_tile(x, food.y, theme.guidance_color);
  }
  for (size_t y = 0; y != model.rows(); ++y) {
    draw_tile(food.x, y, theme.guidance_color);
  }

  draw_tile(food.x, food.y, theme.food_color);

  const auto& snake = model.get_snake();
  const auto head = snake.front();
  draw_tile(head.x, head.y, theme.snake_head_color);
  for (auto i = snake.begin() + 1; i != snake.end(); ++i) {
    draw_tile(i->x, i->y, theme.snake_body_color);
  }

  XSetForeground(display_, gc_, theme.grid_color);
  for (size_t x = 0; x != model.cols(); ++x) {
    for (size_t y = 0; y != model.rows(); ++y) {
      const auto x_coord = x * tile_size + x_offset;
      const auto y_coord = y * tile_size + y_offset;
      XDrawRectangle(display_, win_, gc_, x_coord, y_coord, tile_size, tile_size);
    }
  }

  XSetForeground(display_, gc_, theme.border_color);
  XDrawRectangle(display_, win_, gc_, x_offset - 1, y_offset - 1,
      model.cols() * tile_size + 1, model.rows() * tile_size + 1);
  XFlush(display_);
}

bool snake::x11_view::should_close() const noexcept {
  return should_close_;
}

bool snake::x11_view::is_open() const noexcept {
  return is_open_;
}

snake::x11_view::command snake::x11_view::poll_input() {
  XEvent event{};
  command last_direction_change = command::none;
  while (XPending(display_)) {
    XNextEvent(display_, &event);
    if (event.type == ClientMessage && (Atom)event.xclient.data.l[0] == wm_delete_) {
      std::println(stderr, "WM delete window request");
      should_close_ = true;
      continue;
    }
    if (event.type == KeyPress) {
      const auto keysym = XLookupKeysym(&event.xkey, 0);
      std::println(stderr, "KeyPress [keysym={}]", keysym);
      switch (keysym) {
      case 'q':
        return command::exit;
      case 'k':
      case XK_Up:
        last_direction_change = command::up;
        break;
      case 'j':
      case XK_Down:
        last_direction_change = command::down;
        break;
      case 'h':
      case XK_Left:
        last_direction_change = command::left;
        break;
      case 'l':
      case XK_Right:
        last_direction_change = command::right;
        break;
      case 't':
        return command::switch_theme;
        break;
      }
    }
  }
  return last_direction_change;
}
namespace snake {
  /* clang-format off */
const game_theme themes[] = {
    {"Synthwave Night", 0xCCCCCC, 0x220022, 0x00FFFF, 0x7000FF, 0x050510, 0xFFE000, 0x151525, true, true },
    {"Forest Hacker",   0x83A598, 0x1D2021, 0xB8BB26, 0x98971A, 0x282828, 0xFB4934, 0x3C3836, true, true },
    {"Deep Sea",        0xEEEEEE, 0x001A1A, 0x00FFCC, 0x0088AA, 0x00050A, 0xFF7700, 0x0A1F26, true, true },
    {"Blood Moon",      0xFFFFFF, 0x1A0505, 0xFF0000, 0x800000, 0x0A0000, 0xFFFFFF, 0x221111, true, true },
    {"Acid Classic",    0xFFFFFF, 0x333300, 0x00FF00, 0x00CC00, 0x000000, 0xFF0000, 0x111111, true, false},
};
  /* clang-format on */

  int current_theme_idx = 0;
}

#include <chrono>
#include <memory>
#include <thread>

int main() try {
  snake::game_model model(24, 24);
  std::unique_ptr<snake::game_view> view = std::make_unique<snake::x11_view>();
  if (!view->open()) {
    std::println(stderr, "Failed to open GUI");
    return 1;
  }
  view->render(model, snake::themes[snake::current_theme_idx]);
  // poll
  // step
  // render
  while (true) {
    auto cmd = view->poll_input();
    if (cmd == snake::game_view::command::exit || view->should_close()) {
      break;
    }
    using snake::direction;
    switch (cmd) {
    case snake::game_view::command::up:
      model.set_direction(direction::up);
      break;
    case snake::game_view::command::down:
      model.set_direction(direction::down);
      break;
    case snake::game_view::command::left:
      model.set_direction(direction::left);
      break;
    case snake::game_view::command::right:
      model.set_direction(direction::right);
      break;
    case snake::game_view::command::switch_theme:
      snake::current_theme_idx = (snake::current_theme_idx + 1) %
                                 (sizeof(snake::themes) / sizeof(snake::themes[0]));
      break;
    default:
      break;
    }
    if (model.is_game_over()) {
      view->render(model, snake::themes[snake::current_theme_idx]);
      continue;
    }
    model.step();
    view->render(model, snake::themes[snake::current_theme_idx]);
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    //  std::println(stderr, "Game Over!");
    //  break;
  }
  view->close();

  return 0;
} catch (const std::exception& e) {
  std::println(stderr, "Error: {}", e.what());
  return 1;
}
