cmake_minimum_required(VERSION 3.5)

project(common LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

set(HEADERS
    include/common/definitions.h
    include/common/progress_functions.hpp
    include/common/promt_functions.hpp
    include/common/string_utils.h
    include/common/string_utils.hpp
    include/common/arguments.h
    include/common/arguments.hpp
    #include/common/file_utils.h
    #include/common/file_utils.hpp
)
