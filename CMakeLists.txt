cmake_minimum_required(VERSION 3.5)
project(hse-scheme)

set(CMAKE_CXX_STANDARD 20)

add_library(scheme_src
    src/tokenizer.cpp
    src/parser.cpp
    src/scheme.cpp
    src/object.cpp
    src/operations_impl.cpp
)

add_executable(scheme_repl repl/repl.cpp)
target_link_libraries(scheme_repl scheme_src)
