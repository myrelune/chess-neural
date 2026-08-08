cmake_minimum_required(VERSION 3.20)

project(ChessEngine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(BOOK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/src/polyglot/baron30.bin")
set(HEADER_PATH "${CMAKE_CURRENT_BINARY_DIR}/embedded_book.h")

if(EXISTS ${BOOK_PATH})
    file(READ ${BOOK_PATH} BOOK_HEX HEX)
    string(REGEX REPLACE "(..)" "0x\\1, " BOOK_C_ARRAY "${BOOK_HEX}")
    file(WRITE ${HEADER_PATH} "#pragma once\nnamespace EmbeddedBook {\nconstexpr unsigned char data[] = {\n${BOOK_C_ARRAY}\n};\n}\n")
endif()

add_executable(
    chess
    src/main.cpp
    src/board/board.cpp
    src/movegen/movegen.cpp
    src/uci/uci.cpp
    src/eval/evaluate.cpp
    src/eval/tables.cpp
    src/attacks/attacks.cpp
    src/search/searcher.cpp
    src/zobrist/zobrist.cpp
    src/polyglot/polyglot.cpp
)

target_include_directories(chess PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
