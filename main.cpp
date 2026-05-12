#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstddef>
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
  struct {
    unsigned long border_color;
    unsigned long guidance_color;
    unsigned long snake_head_color;
    unsigned long snake_body_color;
    unsigned long background_color;
    unsigned long food_color;
    unsigned long grid_color;
  } palette;
  bool show_grid;
  bool show_guidance;
};

class snake::game_view {
public:
  enum class command : std::uint8_t {
    none,
    up,
    down,
    left,
    right,
    exit,
    switch_theme,
    restart
  };

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

  bool open_display() noexcept;
  bool open_screen() noexcept;
  bool open_window() noexcept;
  void prepare_window() noexcept;
};

bool snake::x11_view::open() noexcept {
  if (is_open()) {
    return true;
  }
  if (!open_display() || !open_screen() || !open_window()) {
    return false;
  }
  prepare_window();
  std::println(stderr, "X server connection opened");
  std::println(stderr, "Vendor  {}", XServerVendor(display_));
  std::println(stderr, "Release {}", XVendorRelease(display_));
  std::println(stderr, "Display {}", XDisplayString(display_));
  is_open_ = true;
  should_close_ = false;
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
    if (theme.show_grid) {
      XSetForeground(display_, gc_, theme.palette.grid_color);
      XDrawRectangle(display_, win_, gc_, x_coord, y_coord, tile_size, tile_size);
    }
  };

  XSetBackground(display_, gc_, theme.palette.background_color);
  XClearWindow(display_, win_);

  const auto food = model.get_food();
  if (theme.show_guidance) {
    for (size_t x = 0; x != model.cols(); ++x) {
      draw_tile(x, food.y, theme.palette.guidance_color);
    }
    for (size_t y = 0; y != model.rows(); ++y) {
      draw_tile(food.x, y, theme.palette.guidance_color);
    }
  }

  draw_tile(food.x, food.y, theme.palette.food_color);

  const auto& snake = model.get_snake();
  const auto head = snake.front();
  draw_tile(head.x, head.y, theme.palette.snake_head_color);
  for (auto i = snake.begin() + 1; i != snake.end(); ++i) {
    draw_tile(i->x, i->y, theme.palette.snake_body_color);
  }

  if (theme.show_grid) {
    XSetForeground(display_, gc_, theme.palette.grid_color);
    for (size_t x = 0; x != model.cols(); ++x) {
      for (size_t y = 0; y != model.rows(); ++y) {
        const auto x_coord = x * tile_size + x_offset;
        const auto y_coord = y * tile_size + y_offset;
        XDrawRectangle(display_, win_, gc_, x_coord, y_coord, tile_size, tile_size);
      }
    }
  }

  XSetForeground(display_, gc_, theme.palette.border_color);
  XDrawRectangle(display_, win_, gc_, x_offset - 1, y_offset - 1,
      model.cols() * tile_size + 1, model.rows() * tile_size + 1);

  if (model.is_game_over()) {
    static const uint16_t glyphs[256][5] = { ['G'] = { 0b111, 0b100, 0b101, 0b101,
                                                 0b111 },
      ['A'] = { 0b111, 0b101, 0b111, 0b101, 0b101 },
      ['M'] = { 0b101, 0b111, 0b111, 0b101, 0b101 },
      ['E'] = { 0b111, 0b100, 0b111, 0b100, 0b111 },
      ['O'] = { 0b111, 0b101, 0b101, 0b101, 0b111 },
      ['V'] = { 0b101, 0b101, 0b101, 0b101, 0b010 },
      ['R'] = { 0b111, 0b101, 0b110, 0b101, 0b101 },
      [' '] = { 0b000, 0b000, 0b000, 0b000, 0b000 } };
    // XSetForeground(display_, gc_, 0x000000);
    //   XFillRectangle(display_, win_, gc_, attrs.width / 2 - 100, attrs.height / 2 - 30,
    //   200, 60);
    int p_size = std::max(static_cast<unsigned long>(2), tile_size / 4);
    std::string text = "GAME OVER";
    int x_center = attrs.width / 2;
    int y_center = attrs.height / 2;
    int char_w = 3 * p_size;
    int char_h = 5 * p_size;
    int spacing = 1 * p_size;
    int total_w = static_cast<int>(text.size()) * (char_w + spacing) - spacing;

    int cur_x = x_center - total_w / 2;
    int cur_y = y_center - char_h / 2;

    XSetForeground(display_, gc_, 0xFFFFFF);

    for (char c : text) {
      const uint16_t* glyph = glyphs[static_cast<unsigned char>(c)];
      for (int row = 0; row < 5; ++row) {
        for (int col = 0; col < 3; ++col) {
          if (glyph[row] & (1 << (2 - col))) {
            XFillRectangle(display_, win_, gc_, cur_x + col * p_size,
                cur_y + row * p_size, p_size, p_size);
          }
        }
      }
      cur_x += char_w + spacing;
    }
  }
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
      case 'r':
        return command::restart;
      case 't':
        return command::switch_theme;
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
      }
    }
  }
  return last_direction_change;
}

bool snake::x11_view::open_display() noexcept {
  display_ = XOpenDisplay(nullptr);
  if (!display_) {
    std::println(stderr, "Failed to open display");
    return false;
  }
  return true;
}

bool snake::x11_view::open_screen() noexcept {
  assert(display_ && "Display must be opened before screen");
  const auto screen_number = XDefaultScreen(display_);
  screen_ = XScreenOfDisplay(display_, screen_number);
  if (!screen_) {
    std::println(stderr, "Failed to open screen");
    XCloseDisplay(display_);
    return false;
  }
  return true;
}

bool snake::x11_view::open_window() noexcept {
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
  return true;
}

void snake::x11_view::prepare_window() noexcept {
  XEvent event{};
  do {
    XNextEvent(display_, &event);
  } while (event.type != Expose);
}

#include <chrono>
#include <memory>
#include <thread>

namespace snake {
  void apply_command(game_model& model, game_view::command cmd) {
    switch (cmd) {
    case game_view::command::restart:
      model.restart();
      break;
    case game_view::command::up:
      model.set_direction(direction::up);
      break;
    case game_view::command::down:
      model.set_direction(direction::down);
      break;
    case game_view::command::left:
      model.set_direction(direction::left);
      break;
    case game_view::command::right:
      model.set_direction(direction::right);
      break;
    default:
      break;
    }
  }
}

int main() try {
  using namespace snake;
  /* clang-format off */
  const std::vector<game_theme> themes = {
    {"Synthwave Night", {0xCCCCCC, 0x220022, 0x00FFFF, 0x7000FF, 0x050510, 0xFFE000, 0x151525}, false, true },
    {"Forest Hacker",   {0x83A598, 0x1D2021, 0xB8BB26, 0x98971A, 0x282828, 0xFB4934, 0x3C3836}, true, true },
    {"Deep Sea",        {0xEEEEEE, 0x001A1A, 0x00FFCC, 0x0088AA, 0x00050A, 0xFF7700, 0x0A1F26}, true, true },
    {"Blood Moon",      {0xFFFFFF, 0x1A0505, 0xFF0000, 0x800000, 0x0A0000, 0xFFFFFF, 0x221111}, true, true },
    {"Acid Classic",    {0xFFFFFF, 0x000000, 0x00FF00, 0x00FF00, 0x000000, 0xFF0000, 0x000000}, false, false},
  };
  /* clang-format on */
  std::size_t theme_idx = 0;
  game_model model(24, 24);
  std::unique_ptr<game_view> view = std::make_unique<x11_view>();
  if (!view->open()) {
    std::println(stderr, "Failed to open GUI");
    return 1;
  }
  view->render(model, themes[theme_idx]);
  using command = game_view::command;
  constexpr auto frame_time = std::chrono::milliseconds(150);
  while (true) {
    auto time_begin = std::chrono::steady_clock::now();
    const auto cmd = view->poll_input();
    if (cmd == command::exit || view->should_close()) {
      break;
    }
    if (cmd == command::switch_theme) {
      theme_idx = (theme_idx + 1) % themes.size();
    }
    apply_command(model, cmd);
    if (!model.is_game_over()) {
      model.step();
    }
    view->render(model, themes[theme_idx]);
    const auto elapsed = std::chrono::steady_clock::now() - time_begin;
    if (elapsed < frame_time) {
      std::this_thread::sleep_for(frame_time - elapsed);
    }
  }
  view->close();
} catch (const std::exception& e) {
  std::println(stderr, "Error: {}", e.what());
  return 1;
}
