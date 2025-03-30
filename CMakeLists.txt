cmake_minimum_required(VERSION 3.10)

# Project name
project( FileWatch VERSION 0.0.2 LANGUAGES C CXX)

option(BuildTests "Build the unit tests" ON)
enable_testing()

# Enable c++17 required for std::filesystem
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED on)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
   exec_program(uname ARGS -v  OUTPUT_VARIABLE DARWIN_VERSION)
   string(REGEX MATCH "[0-9]+" DARWIN_VERSION ${DARWIN_VERSION})
   if (DARWIN_VERION LESS 17)
     message(FATAL_ERROR "Minimum Darwin version 17 (MacOS 10.13)"
       "required. Current version: ${DARWIN_VERSION}")
   endif()
endif()

find_package(Threads REQUIRED)

add_library(FileWatch INTERFACE)
target_include_directories(FileWatch INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(FileWatch INTERFACE
  $<$<PLATFORM_ID:Linux>:-DFILEWATCH_PLATFORM_LINUX=1>
  $<$<PLATFORM_ID:Darwin>:-DFILEWATCH_PLATFORM_MAC=1>
  $<$<PLATFORM_ID:Windows>:-DFILEWATCH_PLATFORM_WIN=1>
)

target_link_libraries(FileWatch INTERFACE Threads::Threads)
target_link_options(FileWatch INTERFACE   $<$<PLATFORM_ID:Darwin>:-framework CoreServices>)

# dial up the warnings
if(MSVC)
  # Force to always compile with W4
  if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /permissive")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
  endif()
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  # Update if necessary
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-long-long -pedantic")
endif()

# create and configure the unit test target
if(BuildTests)
    add_subdirectory(tests)
endif()

# add_subdirectory(example)
