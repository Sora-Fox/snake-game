#include <cstdint>
#include <vector>

namespace snake {
  struct point_t;
  enum class direction : std::uint8_t;
  class game_model;
  [[nodiscard]] bool operator==(point_t, point_t) noexcept;
}

struct snake::point_t {
  int x;
  int y;
};

enum class snake::direction : std::uint8_t {
  down,
  up,
  left,
  right,
};

class snake::game_model {
public:
  explicit game_model(int rows, int columns);

  void set_direction(direction) noexcept;
  [[nodiscard]] bool step();

  [[nodiscard]] const std::vector<point_t>& get_snake() const noexcept;
  [[nodiscard]] point_t get_food() const noexcept;
  [[nodiscard]] std::size_t get_scores() const noexcept;

private:
  std::vector<point_t> snake_;
  std::size_t scores_;
  point_t food_;
  int max_x_;
  int max_y_;
  direction dir_;

  void generate_initial_snake();
  [[nodiscard]] point_t generate_food() const;
  [[nodiscard]] bool is_snake(point_t) const noexcept;
  [[nodiscard]] bool is_field(point_t) const noexcept;
};
