cmake_minimum_required(VERSION 3.5)

project(opencv_utils LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include (CMakeIncludes.txt)

set(HEADERS
    include/opencv_utils/raster_roi.h
    include/opencv_utils/gdal_bridges.h
    include/opencv_utils/geometry_renderer.h
)

set(SOURCES
    src/raster_roi.cpp
    src/gdal_bridges.cpp
)

add_library(${PROJECT_NAME} ${HEADERS} ${SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC include)

target_link_libraries (
                        ${PROJECT_NAME}
                        gdal_utils
                        eigen_utils
                        ${OpenCV_LIBS}
                        differentiation
                    )
