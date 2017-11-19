cmake_minimum_required(VERSION 3.1)

# Project name
project( FileWatch VERSION 0.0.1 LANGUAGES C CXX)

option(BuildTests "Build the unit tests" ON)
enable_testing()

# Add debug flags
if(MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zi /MP")
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
endif()

# create and configure the unit test target
if(BuildTests)
    add_subdirectory(tests)
endif()