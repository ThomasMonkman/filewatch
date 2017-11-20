#include <string>
using namespace std::string_literals;

#ifdef _WIN32
#define UNICODE
using test_string = std::wstring;
const static auto test_folder_path = L"./"s;
const static auto test_file_name = L"test.txt"s;
#endif // _WIN32

#if __unix__
using test_string = std::string;
const static auto test_folder_path = "./"s;
const static auto test_file_name = "test.txt"s;
#endif // __unix__

#include "catch/catch.hpp"

#include "../FileWatch.hpp"

#include "Util/TestHelper.hpp"

#include <future>
#include <iostream>
#include <fstream>


TEST_CASE("watch for file add", "[added]") {
	std::promise<test_string> promise;
	std::future<test_string> future = promise.get_future();
	filewatch::FileWatch<test_string> watch(test_folder_path, [&promise](const test_string& path, const filewatch::ChangeType change_type) {
		promise.set_value(path);
	});
	std::ofstream test_file(test_file_name);
	test_file << "test" << std::endl;
	test_file.close();

	auto path = TestHelper::get_with_timeout(future);
	REQUIRE(path == test_file_name);
}