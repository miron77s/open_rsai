cmake_minimum_required(VERSION 3.5)

project(open_rsai_core LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

MACRO(SUBDIRLIST result curdir)
  FILE(GLOB children RELATIVE ${curdir} ${curdir}/*)
  SET(dirlist "")
  FOREACH(child ${children})
  IF(IS_DIRECTORY ${curdir}/${child} AND NOT ( ${child} STREQUAL ".git" ) )
      LIST(APPEND dirlist ${child})
    ENDIF()
  ENDFOREACH()
  SET(${result} ${dirlist})
ENDMACRO()

MACRO(CMAKE_DEPS deps file_name)
  FILE(STRINGS ${file_name} file_content )

  SET ( targets_section false )
  SET ( deps_local "" )

  FOREACH(line ${file_content})
    string ( FIND ${line} "target_link_libraries" target_index )
    IF ( ${target_index} GREATER_EQUAL 0 )
      SET ( targets_section true )
    ENDIF()

    IF ( ${targets_section} )
      string ( CONCAT deps_local ${deps_local} ${line} )
    ENDIF()

    string ( FIND ${line} ")" target_index )
    IF ( ${target_index} GREATER_EQUAL 0 )
      SET ( targets_section false )
    ENDIF()

  ENDFOREACH()

  separate_arguments ( deps_local )

  SET ( dep_list "" )
  FOREACH(dep_candidate ${deps_local})
    string ( REGEX MATCHALL "[{}$()]|target_link_libraries" dep ${dep_candidate} )
    IF ( "${dep}" STREQUAL "" )

      LIST(APPEND dep_list ${dep_candidate})
    ENDIF()
  ENDFOREACH()

  SET ( ${deps} ${dep_list} )
ENDMACRO()

SUBDIRLIST(SUBDIRS ${CMAKE_CURRENT_SOURCE_DIR})

message (STATUS "----- Parsing includes -----")
FOREACH(subdir ${SUBDIRS})
  message (STATUS "Parsing ${subdir}")

  if( EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${subdir}/CMakeIncludes.txt )
    include(${subdir}/CMakeIncludes.txt)
  else()
    message (STATUS "No CMakeIncludes.txt in ${subdir}")
  endif()

  if( EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${subdir}/include )
    include_directories(${subdir}/include)
  else()
    message (STATUS "No include dir in ${subdir} using this dir as is")
    include_directories(${subdir})
  endif()
ENDFOREACH()

message (STATUS "----- Making projects -----")
FOREACH(subdir ${SUBDIRS})
  message (STATUS "Parsing ${subdir}")

  if( EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${subdir}/CMakeLists.txt )
      CMAKE_DEPS ( deps ${CMAKE_CURRENT_SOURCE_DIR}/${subdir}/CMakeLists.txt )
      FOREACH(dep ${deps})
        #MESSAGE( STATUS ${dep} )
      ENDFOREACH()
    add_subdirectory (${subdir})
  else()
      message (STATUS "No CMake file in ${subdir}")
  endif()
ENDFOREACH()
