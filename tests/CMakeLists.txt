#include catch
add_library(Catch INTERFACE)
target_include_directories(Catch INTERFACE ${PROJECT_SOURCE_DIR}/tests/catch/catch.hpp)

if(FileWatch_Build_Tests)
    add_subdirectory(string)
endif()

if(FileWatch_Build_Tests_FileSystem)
    add_subdirectory(filesystem)
endif()