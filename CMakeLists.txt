cmake_minimum_required(VERSION 4.0...4.2)
project(snake LANGUAGES CXX)

find_package(Curses REQUIRED)
find_package(X11 REQUIRED)

add_executable(snake main.cpp game_model.cpp)
target_compile_features(snake PRIVATE cxx_std_23)

target_include_directories(snake PRIVATE ${X11_INCLUDE_DIR})
target_include_directories(snake PRIVATE ${CURSES_INCLUDE_DIR})

target_link_libraries(snake PRIVATE ${CURSES_LIBRARIES})
target_link_libraries(snake PRIVATE ${X11_LIBRARIES})

install(TARGETS snake DESTINATION bin)
