cmake_minimum_required(VERSION 3.5)

project(roof_locator_functional LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)


set(HEADERS
    include/rsai/roof_locator.h
    include/rsai/roof_locator.hpp
  )

set(SOURCES
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
                        differentiation
                        markup
                        projection_and_shade_locator_functional
)


