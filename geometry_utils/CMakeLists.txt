cmake_minimum_required(VERSION 3.5)

project(geometry_utils LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include (CMakeIncludes.txt)

set(HEADERS
    include/geometry_utils/points_fill.h
)

set(SOURCES
    src/points_fill.cpp
)

add_library(${PROJECT_NAME} ${HEADERS} ${SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC include)

target_link_libraries (
                        ${PROJECT_NAME}
                        ${Boost_LIBRARIES}
                        eigen_utils
                    )
