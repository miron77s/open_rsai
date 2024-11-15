cmake_minimum_required(VERSION 3.5)

project(markup_writer_functional LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(HEADERS
    include/rsai/markup_writer.h
    include/rsai/markup_writer.hpp
  )

set(SOURCES
    src/markup_writer.cpp
  )

add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS})

set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(${PROJECT_NAME} PUBLIC include)

target_link_libraries(
                        ${PROJECT_NAME}
                        gdal_utils
                        eigen_utils
                        opencv_utils
                        threading_utils
                        building_models
                        markup
                        projection_and_shade_locator_functional
)

