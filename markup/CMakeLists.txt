cmake_minimum_required(VERSION 3.5)

project(markup LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC" )

include (CMakeIncludes.txt)

set(HEADERS
    include/rsai/markup/tile_saver.h
    include/rsai/markup/tile.h
    include/rsai/markup/writers.h
)

set(SOURCES
    src/tile_saver.cpp
    src/tile.cpp
    src/writers.cpp
)

add_library(${PROJECT_NAME} STATIC ${HEADERS} ${SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC include)

target_link_libraries (
                        ${PROJECT_NAME}
                        gdal_utils
                        eigen_utils
                        opencv_utils
                        building_models
                    )
