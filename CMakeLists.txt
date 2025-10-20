cmake_minimum_required(VERSION 3.10)
project(almost_sqlite)

set(CMAKE_CXX_STANDARD 11)

# Только основные файлы (исключаем проблемные)
set(SOURCES
    src/parser.cpp
    src/types.cpp
    src/Process_file.cpp
    src/metadata.cpp
    # src/test_types.cpp    
    # src/Constraint_table.cpp 
)

add_executable(almost_sqlite ${SOURCES})
target_include_directories(almost_sqlite PRIVATE src)

