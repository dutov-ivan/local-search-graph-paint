cmake_minimum_required(VERSION 3.10.0)
project(graph-coloring-local-search VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)


add_executable(GraphColoring
    main.cpp
    init.cpp
	graph.cpp
	algorithms.cpp
    bindings.cpp
)

target_link_options(GraphColoring PRIVATE 
    --bind
    -sMODULARIZE=1
    -sEXPORT_ES6=1
    --emit-tsd "$<TARGET_FILE_DIR:GraphColoring>/GraphColoring.d.ts" # or wherever else you want it to go

)
