Include(FetchContent)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.4.0 # or a later release
)

FetchContent_MakeAvailable(Catch2)
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)

include(CTest)
include(Catch)


set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

#add tests
set(FILEWATCH_UNIT_TEST_TARGET_NAME "filewatch_unit")

add_executable(${FILEWATCH_UNIT_TEST_TARGET_NAME}
  ${PROJECT_SOURCE_DIR}/tests/Test.cpp)

target_link_libraries(${FILEWATCH_UNIT_TEST_TARGET_NAME}
	FileWatch
	Catch2::Catch2WithMain
	Threads::Threads)

add_test(NAME "${FILEWATCH_UNIT_TEST_TARGET_NAME}_default"
	COMMAND ${FILEWATCH_UNIT_TEST_TARGET_NAME}
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# install(TARGETS ${FILEWATCH_UNIT_TEST_TARGET_NAME} DESTINATION ${PROJECT_BINARY_DIR}/bin)
