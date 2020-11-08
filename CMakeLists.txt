cmake_minimum_required(VERSION 3.13)

add_library("stereogram" INTERFACE)
target_include_directories("stereogram" INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
