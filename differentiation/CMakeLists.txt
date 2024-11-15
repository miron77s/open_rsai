cmake_minimum_required(VERSION 3.5)

project(differentiation LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include (CMakeIncludes.txt)

set(HEADERS
    include/differentiation/gauss_directed_derivative.h
    include/differentiation/convolution_mask.h
    include/differentiation/convolution_mask.hpp
    include/differentiation/edge_detection.h
)

set(SOURCES
    src/gauss_directed_derivative.cpp
    src/edge_detection.cpp
)

add_library(${PROJECT_NAME} STATIC ${HEADERS} ${SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC include)

target_link_libraries (
                        ${PROJECT_NAME}
                        ${OpenCV_LIBS}
                    )
