cmake_minimum_required(VERSION 3.5)

project(projection_and_shade_locator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(../functional/include)

set(SOURCES
    src/main.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(
                        ${PROJECT_NAME}
                        projection_and_shade_locator_functional
)


set_target_properties(${PROJECT_NAME} PROPERTIES
                      RUNTIME_OUTPUT_DIRECTORY_DEBUG "../../bin/commands"
                      RUNTIME_OUTPUT_DIRECTORY_RELEASE "../../bin/commands"
)

install(TARGETS ${PROJECT_NAME} DESTINATION)
