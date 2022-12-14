#include <string>
#include <regex>
#ifdef _WIN32
#define _UNICODE
#define UNICODE
using test_string = std::wstring;
using test_char = wchar_t*;
using test_regex = std::wregex;
#endif // _WIN32

#if __unix__ || __APPLE__
using test_string = std::string;
using test_char = char*;
using test_regex = std::regex;
#endif // __unix__

#include "catch/catch.hpp"

#include "../FileWatch.hpp"

#include "Util/TestHelper.hpp"

#include <future>
#include <algorithm>
#include <mutex>
#include <vector>
#include <set>
#include <thread>

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


TEST_CASE("copy constructor", "[constructors]") {
	const auto test_folder_path = testhelper::cross_platform_string("./");
	const auto test_file_name = testhelper::cross_platform_string("test.txt");
	
	std::promise<void> promise;
	std::future<void> future = promise.get_future();
	std::vector<test_string> files_triggered;
	std::set<std::thread::id> file_watch_threads;
	std::mutex mutex;
	const auto expected_triggers = 2u;

	filewatch::FileWatch<test_string> watch(test_folder_path, [&promise, &files_triggered, &file_watch_threads, &expected_triggers, &mutex](const test_string& path, const filewatch::Event change_type) {
		std::lock_guard<std::mutex> lock(mutex);
		file_watch_threads.insert(std::this_thread::get_id());
		files_triggered.push_back(path);
		if (file_watch_threads.size() == expected_triggers) {
			promise.set_value();
		}
	});

	filewatch::FileWatch<test_string> watch2(watch);

	testhelper::create_and_modify_file(test_file_name);

	testhelper::get_with_timeout(future);
	const auto files_match = std::all_of(files_triggered.begin(), files_triggered.end(), [&test_file_name](const test_string& path) { return path == test_file_name; });
	REQUIRE(files_match);
}


TEST_CASE("copy assignment operator", "[operator]") {
	const auto test_folder_path = testhelper::cross_platform_string("./");
	const auto test_file_name = testhelper::cross_platform_string("test.txt");

	std::promise<void> promise;
	std::future<void> future = promise.get_future();
	std::vector<test_string> files_triggered;
	std::set<std::thread::id> file_watch_threads;
	std::mutex mutex;
	const auto expected_triggers = 2u;

	filewatch::FileWatch<test_string> watch(test_folder_path, [&promise, &files_triggered, &file_watch_threads, &expected_triggers, &mutex](const test_string& path, const filewatch::Event change_type) {
		std::lock_guard<std::mutex> lock(mutex);
		file_watch_threads.insert(std::this_thread::get_id());
		files_triggered.push_back(path);
		if (file_watch_threads.size() == expected_triggers) {
			promise.set_value();
		}
	});

	filewatch::FileWatch<test_string> watch2 = watch;

	testhelper::create_and_modify_file(test_file_name);

	testhelper::get_with_timeout(future);
	const auto files_match = std::all_of(files_triggered.begin(), files_triggered.end(), [&test_file_name](const test_string& path) { return path == test_file_name; });
	REQUIRE(files_match);
}

TEST_CASE("regex", "[regex]") {
	const auto test_folder_path = testhelper::cross_platform_string("./");
	const auto test_ignore_path = testhelper::cross_platform_string("./ignore.txt");
	const auto test_file_name = testhelper::cross_platform_string("test.txt");
	// create the file otherwise the Filewatch will throw
	testhelper::create_and_modify_file(test_file_name);

	std::promise<test_string> promise;
	std::future<test_string> future = promise.get_future();

	filewatch::FileWatch<test_string> watch(test_folder_path, test_regex(testhelper::cross_platform_string("test.*")),[&promise, &test_file_name](const test_string& path, const filewatch::Event change_type) {
		REQUIRE(path == test_file_name);
		promise.set_value(path);
	});

	testhelper::create_and_modify_file(test_ignore_path);
	testhelper::create_and_modify_file(test_file_name);

	auto path = testhelper::get_with_timeout(future);
	REQUIRE(path == test_file_name);
}

#ifdef _WIN32
//TEST_CASE("base type", "[char]") {
//	const auto test_folder_path = _T("./");
//	const auto test_file_name = _T("test.txt");
//
//	std::promise<test_char> promise;
//	std::future<test_char> future = promise.get_future();
//	filewatch::FileWatch<test_char> watch(test_folder_path, [&promise](const test_char& path, const filewatch::Event change_type) {
//		promise.set_value(path);
//	});
//
//	testhelper::create_and_modify_file(test_file_name);
//
//	auto path = testhelper::get_with_timeout(future);
//	REQUIRE(path == test_file_name);
//}
#endif