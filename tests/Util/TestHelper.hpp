#ifndef TESTHELPER_HPP
#define TESTHELPER_HPP
#include <chrono>
#include <ratio>
#include <future>
#include <stdexcept>
#include <string>
#include <utility>
#include <iostream>
#include <fstream>

namespace testhelper {
	namespace config {
		//3 second test timeout
		 typedef std::chrono::duration<float, std::ratio_multiply<std::chrono::seconds::period, std::ratio<100>>> test_timeout;
	}
	template<typename T>
	static T get_with_timeout(std::future<T>& future_to_wait_for)
	{
		const auto status = future_to_wait_for.wait_for(testhelper::config::test_timeout(1));
		REQUIRE(status != std::future_status::deferred);
		REQUIRE(status != std::future_status::timeout);
		if (status == std::future_status::ready) {
			return future_to_wait_for.get();
		}
		else {
			throw std::runtime_error("Timeout reached");
		}
	}

#if defined _WIN32 && (defined UNICODE || defined _UNICODE)
	static std::wstring cross_platform_string(std::string&& string) 
	{ 
		// deal with trivial case of empty string
		if (string.empty())    return std::wstring();
		// determine required length of new string
		size_t reqLength = ::MultiByteToWideChar(CP_UTF8, 0, string.c_str(), static_cast<int>(string.length()), 0, 0);
		// construct new string of required length
		std::wstring ret(reqLength, L'\0');
		// convert old string to new string
		::MultiByteToWideChar(CP_UTF8, 0, string.c_str(), static_cast<int>(string.length()), &ret[0], (int)ret.length());
		// return new string ( compiler should optimize this away )
		return ret;
	}
#else
	static std::string cross_platform_string(std::string&& string)
	{
		return std::forward<std::string>(string);
	}
#endif

	template<typename T>
	static void create_and_modify_file(T& path)
	{
		std::ofstream file(path);
		file << "test" << std::endl;
		file.close();
	}
}
#endif