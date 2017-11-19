#ifdef _WIN32
#define UNICODE
#endif // _WIN32
#include "catch/catch.hpp"

#include "../FileWatch.hpp"

#include "Util/TestHelper.hpp"

#include <experimental/filesystem>
#include <future>
#include <iostream>
#include <fstream>

using namespace std::string_literals;

TEST_CASE("Factorials are computed", "[factorial]") {
	std::promise<std::experimental::filesystem::path> promise;
	std::future<std::experimental::filesystem::path> future = promise.get_future();
	filewatch::FileWatch<std::experimental::filesystem::path> watch(L"./"s, [&promise](const std::experimental::filesystem::path& path, const filewatch::ChangeType change_type) {
		promise.set_value(path);
	});

	std::ofstream outfile("test.txt");
	outfile << "my text here!" << std::endl;
	outfile.close();

	auto path = TestHelper::get_with_timeout(future);
	REQUIRE(path == std::experimental::filesystem::path("test.txt"));
}