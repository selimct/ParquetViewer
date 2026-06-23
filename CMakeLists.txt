cmake_minimum_required(VERSION 3.24)

project(ParquetViewer
    VERSION 0.1.0
    LANGUAGES CXX
)

include(GNUInstallDirs)

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

install(TARGETS parquet-viewer RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES packaging/linux/parquet-viewer.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
)
install(FILES packaging/linux/parquet-viewer.svg
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
)

set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME "parquet-viewer")
set(CPACK_PACKAGE_VENDOR "ParquetViewer")
set(CPACK_PACKAGE_CONTACT "ParquetViewer maintainers")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Desktop Parquet file viewer")
set(CPACK_PACKAGE_DESCRIPTION
    "Parquet Viewer is a Qt 6 desktop application for opening, inspecting, and filtering Parquet files using DuckDB."
)
set(CPACK_PACKAGE_DIRECTORY "${CMAKE_BINARY_DIR}/package")
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_DEPENDS "duckdb")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)

include(CPack)
