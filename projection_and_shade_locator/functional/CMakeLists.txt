cmake_minimum_required(VERSION 3.5)

project(projection_and_shade_locator_functional LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)


set(HEADERS
    include/rsai/projection_and_shade_locator.h
    include/rsai/projection_and_shade_locator.hpp
    include/rsai/building_variants_saver.h
    include/rsai/sam_segmentor.h
    include/rsai/sam_segmentor.hpp
  )

set(SOURCES
    src/projection_and_shade_locator.cpp
    src/building_variants_saver.cpp
    src/sam_segmentor.cpp
  )

add_library(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC include)

target_link_libraries(
                        ${PROJECT_NAME}
                        gdal_utils
                        eigen_utils
                        opencv_utils
                        threading_utils
                        building_models
                        differentiation
                        markup
)


