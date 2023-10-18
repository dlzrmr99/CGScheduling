cmake_minimum_required(VERSION 3.16)
project(CGSchedulingC)

set(CMAKE_CXX_STANDARD 17)

link_directories(z3/bin)
add_executable(CGSchedulingC Co1.cpp CoU.cpp SMT.cpp main.cpp)

target_include_directories(CGSchedulingC PUBLIC z3/src/api/c++)
target_include_directories(CGSchedulingC PUBLIC z3/src/api)
target_link_libraries(CGSchedulingC PUBLIC z3)