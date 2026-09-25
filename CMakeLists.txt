cmake_minimum_required(VERSION 3.16)
project(shitwave VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(X11 REQUIRED)

add_executable(shitwave src/main.cpp)
target_include_directories(shitwave PRIVATE ${X11_INCLUDE_DIR})
target_link_libraries(shitwave PRIVATE ${X11_LIBRARIES})
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(shitwave PRIVATE -Wall -Wextra -Wpedantic -O2)
endif()

install(TARGETS shitwave RUNTIME DESTINATION bin)
