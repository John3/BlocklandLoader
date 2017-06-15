cmake_minimum_required(VERSION 3.0)
project(BlocklandLoader)
set(CMAKE_CXX_FLAGS "-static -static-libgcc -static-libstdc++")
add_library(BlocklandLoader SHARED
	src/main.cpp
	src/detours/hde.cpp)
target_link_libraries(BlocklandLoader psapi shlwapi)
