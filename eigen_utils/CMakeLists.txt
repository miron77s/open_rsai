cmake_minimum_required(VERSION 3.5)

project(eigen_utils LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include (CMakeIncludes.txt)

set(HEADERS
    include/eigen_utils/gdal_bridges.h
    include/eigen_utils/vector_helpers.h
    include/eigen_utils/vector_helpers.hpp
    include/eigen_utils/math.hpp
    include/eigen_utils/geometry.h
)

set(SOURCES
    src/gdal_bridges.cpp
    src/geometry.cpp
)

add_library(${PROJECT_NAME} ${HEADERS} ${SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC include)

target_link_libraries (
                        ${PROJECT_NAME}
                        gdal_utils
                        Eigen3::Eigen
                    )
