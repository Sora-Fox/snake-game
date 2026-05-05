#include "game_model.hpp"
#include <algorithm>
#include <random>
#include <ranges>

namespace snake {
  [[nodiscard]] point_t apply_direction(point_t, direction);
}

snake::game_model::game_model(const int rows, const int columns) :
  snake_(),
  scores_(0),
  max_x_(columns - 1),
  max_y_(rows - 1),
  prev_dir_(direction::right),
  dir_(direction::right),
  is_game_over_(false) {
  generate_initial_snake();
  food_ = generate_food();
}

void snake::game_model::set_direction(const direction dir) noexcept {
  auto is_invalid = false;
  is_invalid = is_invalid || (prev_dir_ == direction::up && dir == direction::down);
  is_invalid = is_invalid || (prev_dir_ == direction::down && dir == direction::up);
  is_invalid = is_invalid || (prev_dir_ == direction::left && dir == direction::right);
  is_invalid = is_invalid || (prev_dir_ == direction::right && dir == direction::left);
  if (is_invalid) {
    return;
  }
  dir_ = dir;
}

void snake::game_model::step() {
  if (is_game_over_) {
    return;
  }
  const auto new_head = apply_direction(snake_.front(), dir_);
  if (!is_field(new_head) || (is_snake(new_head) && new_head != snake_.back())) {
    is_game_over_ = true;
    return;
  }
  snake_.insert(snake_.begin(), new_head);
  if (new_head == food_) {
    scores_ += 5;
    food_ = generate_food();
  } else {
    snake_.pop_back();
  }
  prev_dir_ = dir_;
}

std::size_t snake::game_model::rows() const noexcept {
  return max_y_ + 1;
}

std::size_t snake::game_model::cols() const noexcept {
  return max_x_ + 1;
}

const std::vector<snake::point_t>& snake::game_model::get_snake() const noexcept {
  return snake_;
}

snake::point_t snake::game_model::get_food() const noexcept {
  return food_;
}

std::size_t snake::game_model::get_scores() const noexcept {
  return scores_;
}

bool snake::game_model::is_game_over() const noexcept {
  return is_game_over_;
}

void snake::game_model::generate_initial_snake() {
  const auto [mid_x, mid_y] = point_t{ max_x_ / 2, max_y_ / 2 };
  snake_.emplace_back(mid_x + 2, mid_y);
  snake_.emplace_back(mid_x + 1, mid_y);
  snake_.emplace_back(mid_x + 0, mid_y);
  snake_.emplace_back(mid_x - 1, mid_y);
  snake_.emplace_back(mid_x - 2, mid_y);
}

snake::point_t snake::game_model::generate_food() const {
  static auto rd = std::random_device{};
  static auto engine = std::mt19937(rd());
  auto distr_x = std::uniform_int_distribution(0, max_x_);
  auto distr_y = std::uniform_int_distribution(0, max_y_);
  auto food = point_t{};
  do {
    food = point_t{ distr_x(engine), distr_y(engine) };
  } while (is_snake(food));
  return food;
}

bool snake::game_model::is_snake(const point_t point) const noexcept {
  return std::ranges::contains(snake_, point);
}

bool snake::game_model::is_field(const point_t point) const noexcept {
  return std::min(point.x, point.y) >= 0 && point.y <= max_y_ && point.x <= max_x_;
}

bool snake::operator==(const point_t lhs, const point_t rhs) noexcept {
  return lhs.x == rhs.x && lhs.y == rhs.y;
}

snake::point_t snake::apply_direction(const point_t point, const direction dir) {
  auto result = point;
  switch (dir) {
  case direction::up:
    --result.y;
    break;
  case direction::down:
    ++result.y;
    break;
  case direction::left:
    --result.x;
    break;
  case direction::right:
    ++result.x;
    break;
  }
  return result;
}
