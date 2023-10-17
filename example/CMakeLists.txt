add_executable(
      main
      main.cpp
)

target_include_directories(main PRIVATE ${CMAKE_SOURCE_DIR})

if (UNIX)
      target_link_libraries(main pthread)
endif()
