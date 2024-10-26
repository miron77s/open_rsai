cmake_minimum_required(VERSION 3.5)

project(building_models LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include (CMakeIncludes.txt)

set(HEADERS
    include/rsai/building_models/abstract.h
    include/rsai/building_models/prismatic.h
    include/rsai/building_models/roof_estimator.h
    include/rsai/building_models/structure_estimator.h
    include/rsai/building_models/multiview_estimator.h
    include/rsai/building_models/building_renderer.h
)

set(SOURCES
    src/abstract.cpp
    src/prismatic.cpp
    src/roof_estimator.cpp
    src/structure_estimator.cpp
    src/multiview_estimator.cpp
    src/building_renderer.cpp
)

add_library(${PROJECT_NAME} ${HEADERS} ${SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC include)

target_link_libraries (
                        ${PROJECT_NAME}
                        gdal_utils
                        eigen_utils
                        opencv_utils
                        ${OpenCV_LIBS}
                    )
