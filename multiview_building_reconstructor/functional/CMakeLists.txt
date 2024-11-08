cmake_minimum_required(VERSION 3.5)

project(multiview_building_reconstructor_functional LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(HEADERS
    include/rsai/multiview_building_reconstructor.h
    include/rsai/multiview_building_reconstructor.hpp
  )

set(SOURCES
    src/multiview_building_reconstructor.cpp
  )

add_library(${PROJECT_NAME} ${SOURCES} ${HEADERS})

set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

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
                        projection_and_shade_locator_functional
)

