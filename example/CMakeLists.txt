cmake_minimum_required(VERSION 3.10)
project(PongGame)

find_package(SDL2 REQUIRED)

add_executable(pong main.cpp)
target_include_directories(pong PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(pong PRIVATE ${SDL2_LIBRARIES})


