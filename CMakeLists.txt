cmake_minimum_required(VERSION 3.16)
project(sbox_ga_replica CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

find_package(Threads REQUIRED)

add_library(sbox_core cpp/src/sbox.cpp cpp/src/walsh.cpp cpp/src/ga.cpp)
target_include_directories(sbox_core PUBLIC cpp/include)
target_compile_options(sbox_core PRIVATE -Wall -Wextra)

add_executable(sbox_ga cpp/src/main.cpp)
target_link_libraries(sbox_ga PRIVATE sbox_core Threads::Threads)

add_executable(test_core cpp/tests/test_core.cpp)
target_link_libraries(test_core PRIVATE sbox_core)
