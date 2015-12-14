project(hse-sample-project)
cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11")

include_directories(.)

add_library(
  rsync rsync.cpp
)

add_executable(
  main main.cpp
)
target_link_libraries(main rsync pthread boost_serialization)
