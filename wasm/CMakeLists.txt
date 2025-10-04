cmake_minimum_required(VERSION 3.10.0)
project(graph-coloring-local-search VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)

if (MSVC)
    # For Visual Studio
    add_compile_options(/O2 /DNDEBUG)
else()
    # For GCC/Clang
    add_compile_options(-O3 -DNDEBUG)
endif()


add_executable(graph-coloring-local-search
	main.cpp
	graph.cpp
	algorithms.cpp
	visualization.cpp
)

target_link_options(graph-coloring-local-search PRIVATE --bind)

