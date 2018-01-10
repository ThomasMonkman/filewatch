#include <string>
using namespace std::string_literals;
#ifdef _WIN32
#define _UNICODE
#define UNICODE
using test_string = std::wstring;
#endif // _WIN32

#if __unix__
using test_string = std::string;
#endif // __unix__

#include "catch/catch.hpp"

#include "../FileWatch.hpp"

#include "Util/TestHelper.hpp"

#include <future>


TEST_CASE("watch for file add", "[added]") {
	const auto test_folder_path = testhelper::cross_platform_string("./");
	const auto test_file_name = testhelper::cross_platform_string("test.txt");

	std::promise<test_string> promise;
	std::future<test_string> future = promise.get_future();
	filewatch::FileWatch<test_string> watch(test_folder_path, [&promise](const test_string& path, const filewatch::Event change_type) {
		promise.set_value(path);
	});

	testhelper::create_and_modify_file(test_file_name);

	auto path = testhelper::get_with_timeout(future);
	REQUIRE(path == test_file_name);
}

TEST_CASE("single file", "[single-file]") {
	const auto test_folder_path = testhelper::cross_platform_string("./test.txt");
	const auto test_ignore_path = testhelper::cross_platform_string("./ignore.txt");
	const auto test_file_name = testhelper::cross_platform_string("test.txt");
	// create the file otherwise the Filewatch will throw
	testhelper::create_and_modify_file(test_file_name);

	std::promise<test_string> promise;
	std::future<test_string> future = promise.get_future();

	filewatch::FileWatch<test_string> watch(test_folder_path, [&promise, &test_file_name](const test_string& path, const filewatch::Event change_type) {
		REQUIRE(path == test_file_name);
		promise.set_value(path);
	});

	testhelper::create_and_modify_file(test_ignore_path);
	testhelper::create_and_modify_file(test_file_name);


	auto path = testhelper::get_with_timeout(future);
	REQUIRE(path == test_file_name);
}