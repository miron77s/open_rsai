cmake_minimum_required(VERSION 3.5)

project(raster_inliers_extractor_functional LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(HEADERS
    include/rsai/raster_inliers_extractor.h
    include/rsai/raster_inliers_extractor.hpp
  )

set(SOURCES
  )

add_library(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC include)

target_link_libraries(
                        ${PROJECT_NAME}
                        gdal_utils
                        eigen_utils

)

