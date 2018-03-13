cmake_minimum_required (VERSION 3.0)
project (ssocks)

configure_file(${CMAKE_SOURCE_DIR}/config.h ${CMAKE_SOURCE_DIR}/src/config.h)
add_subdirectory(src)