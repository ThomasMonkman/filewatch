#include catch
add_library(Catch INTERFACE)
target_include_directories(Catch INTERFACE ${PROJECT_SOURCE_DIR}/tests/catch/catch.hpp)

#add tests
set(FILEWATCH_UNIT_TEST_TARGET_NAME "filewatch_unit")
add_executable(${FILEWATCH_UNIT_TEST_TARGET_NAME}
	${PROJECT_SOURCE_DIR}/tests/CatchMain.cpp
	${PROJECT_SOURCE_DIR}/tests/Test.cpp)

#target_compile_features(${FILEWATCH_UNIT_TEST_TARGET_NAME} PRIVATE cxx_std_14)

#enable pthreads for linux
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
	
target_link_libraries(${FILEWATCH_UNIT_TEST_TARGET_NAME}
	Catch
	Threads::Threads)

add_test(NAME "${FILEWATCH_UNIT_TEST_TARGET_NAME}_default"
	COMMAND ${FILEWATCH_UNIT_TEST_TARGET_NAME}
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

install(TARGETS ${FILEWATCH_UNIT_TEST_TARGET_NAME} DESTINATION ${PROJECT_BINARY_DIR}/bin)