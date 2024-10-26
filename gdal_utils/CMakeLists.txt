cmake_minimum_required(VERSION 3.5)

project(gdal_utils LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include (CMakeIncludes.txt)

set(HEADERS
    include/gdal_utils/shared_dataset.h
    include/gdal_utils/shared_spatial_reference.h
    include/gdal_utils/shared_feature.h
    include/gdal_utils/shared_geometry.h
    include/gdal_utils/shared_geometry.hpp
    include/gdal_utils/shared_options.h
    include/gdal_utils/shared_spatial_reference.hpp
    include/gdal_utils/helper.h
    include/gdal_utils/create_helpers.h
    include/gdal_utils/open_helpers.h
    include/gdal_utils/all_helpers.h
    include/gdal_utils/helper.hpp
    include/gdal_utils/open_helpers.hpp
    include/gdal_utils/create_helpers.hpp
    include/gdal_utils/region_of_interest.h
    include/gdal_utils/operations.h
    include/gdal_utils/layers.h
)

set(SOURCES
    src/shared_dataset.cpp
    src/shared_spatial_reference.cpp
    src/shared_options.cpp
    src/region_of_interest.cpp
    src/shared_feature.cpp
    src/layers.cpp
)

add_library(${PROJECT_NAME} ${HEADERS} ${SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC include)

target_link_libraries (
                        ${PROJECT_NAME}
                        gdal
                    )
