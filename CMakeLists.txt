cmake_minimum_required(VERSION 3.0)

project(screenshot C)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

include_directories(${GTK3_INCLUDE_DIRS})
link_directories(${GTK3_LIBRARY_DIRS})
add_definitions(${GTK3_CFLAGS_OTHER})

file(GLOB src "*.c")

set(CMAKE_C_FLAGS "-fbounds-check -Ofast")

add_executable(screenshot ${src})

target_link_libraries(screenshot ${GTK3_LIBRARIES} m Xrandr Xext X11)