cmake_minimum_required(VERSION 3.20)

project(ChessEngine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(
    chess
    src/main.cpp
    src/board/board.cpp
    src/movegen/movegen.cpp
    src/uci/uci.cpp
    src/eval/evaluate.cpp
    src/eval/tables.cpp
    src/attacks/attacks.cpp
)
