#include <chrono>
#include <cstdio>
#include <print>
#include <stdexcept>
#include <thread>

#include <X11/Xlib.h>
#include <X11/keysym.h>

#include "game_model.hpp"

namespace snake {
  class x11_gui final {
  public:
    x11_gui();
    ~x11_gui();

    void run();

  private:
    Display* const display_ = nullptr;
    Screen* const screen_ = nullptr;
    Window win_{};
    GC gc_{};
    game_model model_;
    Atom wm_delete_;
    int tile_size_ = 0;
    int x_offset_ = 0;
    int y_offset_ = 0;
    bool is_running_ = false;

    void update_tile_size_and_offset();
    void close_connection() noexcept;
    void perform_step();

    void handle_event(XEvent&);
    void handle_expose(const XEvent&);
    void handle_keypress(XEvent&);

    void draw_tile(int x, int y, unsigned long color);
    void clear_tile(int x, int y);
    void draw_snake_head_tile(int x, int y);
    void draw_snake_body_tile(int x, int y);

    void draw_snake();
    void draw_food();
    void draw_border();
    void draw_frame();
  };
}

namespace {
  Display* open_display();
  Window create_window(Display*, Screen*);
  Screen* get_screen(Display*);
}

snake::x11_gui::x11_gui() :
  display_(open_display()),
  screen_(get_screen(display_)),
  win_(create_window(display_, screen_)),
  gc_(XCreateGC(display_, win_, 0, nullptr)),
  model_(24, 24),
  wm_delete_(XInternAtom(display_, "WM_DELETE_WINDOW", false)),
  is_running_(false) {
  XSelectInput(display_, win_, ExposureMask | KeyPressMask);
  XSetWMProtocols(display_, win_, &wm_delete_, 1);
  XMapWindow(display_, win_);
  XStoreName(display_, win_, "Snake Game");
  update_tile_size_and_offset();
  std::println(stderr, "Vendor  {}", XServerVendor(display_));
  std::println(stderr, "Release {}", XVendorRelease(display_));
  std::println(stderr, "Display {}", XDisplayString(display_));
}

snake::x11_gui::~x11_gui() {
  close_connection();
}

void snake::x11_gui::close_connection() noexcept {
  XFreeGC(display_, gc_);
  XDestroyWindow(display_, win_);
  XCloseDisplay(display_);
  std::println(stderr, "X connection was closed");
}

void snake::x11_gui::run() {
  is_running_ = true;
  auto event = XEvent{};
  XNextEvent(display_, &event);
  if (event.type != Expose) {
    std::println(stderr, "Unexpected first event");
  }
  draw_frame();
  XFlush(display_);
  while (is_running_) {
    while (is_running_ && XPending(display_)) {
      XNextEvent(display_, &event);
      handle_event(event); // may set is_running = false
    }
    /* clang-format off */ if (!is_running_) { break;}
    perform_step();  // may set is_running = false
    if (!is_running_) { break;} /* clang-format on */
    XFlush(display_);
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
  }
}

void snake::x11_gui::perform_step() {
  const auto old_food = model_.get_food();
  const auto old_tail = model_.get_snake().back();
  const auto old_head = model_.get_snake().front();
  model_.step();
  if (model_.is_game_over()) {
    is_running_ = false;
    return;
  }
  const auto new_head = model_.get_snake().front();
  if (new_head == old_food) {
    draw_food();
  } else {
    clear_tile(old_tail.x, old_tail.y);
  }
  draw_snake_body_tile(old_head.x, old_head.y);
  draw_snake_head_tile(new_head.x, new_head.y);
}

void snake::x11_gui::handle_event(XEvent& event) {
  if (event.type == ClientMessage && (Atom)event.xclient.data.l[0] == wm_delete_) {
    std::println(stderr, "WM delete window request");
    is_running_ = false;
    return;
  }
  switch (event.type) {
  case Expose:
    handle_expose(event);
    break;
  case KeyPress:
    handle_keypress(event);
    break;
  }
}

void snake::x11_gui::handle_expose(const XEvent& event) {
  if (event.xexpose.count != 0) {
    return;
  }
  std::println(stderr, "Expose");
  update_tile_size_and_offset();
  draw_frame();
}

void snake::x11_gui::handle_keypress(XEvent& event) {
  const auto keysym = XLookupKeysym(&event.xkey, 0);
  std::println(stderr, "KeyPress [keysym={}]", keysym);
  switch (keysym) {
  case 'q':
    is_running_ = false;
    break;
  case 'k':
  case XK_Up:
    model_.set_direction(direction::up);
    break;
  case 'j':
  case XK_Down:
    model_.set_direction(direction::down);
    break;
  case 'h':
  case XK_Left:
    model_.set_direction(direction::left);
    break;
  case 'l':
  case XK_Right:
    model_.set_direction(direction::right);
    break;
  }
}

void snake::x11_gui::update_tile_size_and_offset() {
  auto [h, w] = std::pair(model_.rows(), model_.cols());
  auto attrs = XWindowAttributes{};
  XGetWindowAttributes(display_, win_, &attrs);
  tile_size_ = std::min(attrs.width / (w + 1), attrs.height / (h + 1));
  x_offset_ = (attrs.width - 1 - tile_size_ * w) / 2;
  y_offset_ = (attrs.height - 1 - tile_size_ * h) / 2;
}

void snake::x11_gui::draw_tile(int x, int y, unsigned long color) {
  const auto x_coord = x * tile_size_ + x_offset_;
  const auto y_coord = y * tile_size_ + y_offset_;
  XSetForeground(display_, gc_, color);
  XFillRectangle(display_, win_, gc_, x_coord, y_coord, tile_size_, tile_size_);
}

void snake::x11_gui::clear_tile(int x, int y) {
  const auto x_coord = x * tile_size_ + x_offset_;
  const auto y_coord = y * tile_size_ + y_offset_;
  XClearArea(display_, win_, x_coord, y_coord, tile_size_, tile_size_, false);
}

void snake::x11_gui::draw_snake_head_tile(int x, int y) {
  const auto color = 0xFFFF00;
  draw_tile(x, y, color);
}

void snake::x11_gui::draw_snake_body_tile(int x, int y) {
  const auto color = 0x00FF00;
  draw_tile(x, y, color);
}

void snake::x11_gui::draw_snake() {
  const auto& snake = model_.get_snake();
  const auto head = snake.front();
  draw_snake_head_tile(head.x, head.y);
  for (auto i = snake.begin() + 1; i != snake.end(); ++i) {
    draw_snake_body_tile(i->x, i->y);
  }
}

void snake::x11_gui::draw_food() {
  const static auto color = 0xFF0000;
  const auto food = model_.get_food();
  draw_tile(food.x, food.y, color);
}

void snake::x11_gui::draw_border() {
  const static auto white = XWhitePixelOfScreen(screen_);
  XSetForeground(display_, gc_, white);
  XDrawRectangle(display_, win_, gc_, x_offset_ - 1, y_offset_ - 1,
      model_.cols() * tile_size_ + 1, model_.rows() * tile_size_ + 1);
}

void snake::x11_gui::draw_frame() {
  XClearWindow(display_, win_);
  draw_snake();
  draw_food();
  draw_border();
}

namespace {
  Display* open_display() {
    Display* display = XOpenDisplay(nullptr);
    if (!display) {
      throw std::runtime_error("XOpenDisplay faild");
    }
    return display;
  }

  Window create_window(Display* display, Screen* screen) {
    const auto black = XBlackPixelOfScreen(screen);
    const auto root = XRootWindowOfScreen(screen);
    constexpr static auto h = 832;
    constexpr static auto w = 832;
    return XCreateSimpleWindow(display, root, 0, 0, w, h, 0, black, black);
  }

  Screen* get_screen(Display* display) {
    const auto screen_number = XDefaultScreen(display);
    Screen* screen = XScreenOfDisplay(display, screen_number);
    if (!screen) {
      XCloseDisplay(display);
      throw std::runtime_error("XScreenOfDisplay faild");
    }
    return screen;
  }

}

int main() try {
  snake::x11_gui gui;
  gui.run();
} catch (const std::exception& e) {
  std::println(stderr, "Error: {}", e.what());
  return 1;
}
