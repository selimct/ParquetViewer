cmake_minimum_required(VERSION 3.24)

project(ParquetViewer
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_program(DUCKDB_EXECUTABLE duckdb REQUIRED)

add_executable(parquet-viewer
    src/FileTab.cpp
    src/FileTab.h
    src/FilterBuilderWidget.cpp
    src/FilterBuilderWidget.h
    src/FilterCondition.h
    src/MainWindow.cpp
    src/MainWindow.h
    src/ParquetDocument.cpp
    src/ParquetDocument.h
    src/ParquetTableModel.cpp
    src/ParquetTableModel.h
    src/main.cpp
)

target_link_libraries(parquet-viewer
    PRIVATE
        Qt6::Widgets
)

target_compile_definitions(parquet-viewer
    PRIVATE
        DUCKDB_EXECUTABLE="${DUCKDB_EXECUTABLE}"
)

target_compile_options(parquet-viewer PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
)

install(TARGETS parquet-viewer RUNTIME DESTINATION bin)
