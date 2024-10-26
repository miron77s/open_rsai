cmake_minimum_required(VERSION 3.5)

project(threading_utils LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include (CMakeIncludes.txt)

set(HEADERS
    include/threading_utils/thread_safe_feature_layer.h
    include/threading_utils/thread_pool.h
    include/threading_utils/gdal_iterators.h
    include/threading_utils/gdal_iterators.hpp
)

set(SOURCES
    src/thread_safe_feature_layer.cpp
    src/thread_pool.cpp
)

add_library(${PROJECT_NAME} ${HEADERS} ${SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC include)

target_link_libraries(
                        ${PROJECT_NAME}
                        gdal_utils
)
