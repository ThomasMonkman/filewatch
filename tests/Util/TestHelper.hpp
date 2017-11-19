#ifndef TESTHELPER_HPP
#define TESTHELPER_HPP
#include <chrono>
#include <ratio>
#include <future>
#include <stdexcept>

namespace TestHelper {
	namespace Config {
		//3 second test timeout
		typedef std::chrono::duration<float, std::ratio_multiply<std::chrono::seconds::period, std::ratio<100>>> test_timeout;
	}
	template<typename T>
	static T get_with_timeout(std::future<T>& future_to_wait_for)
	{
		const auto status = future_to_wait_for.wait_for(TestHelper::Config::test_timeout(1));
		REQUIRE(status != std::future_status::deferred);
		REQUIRE(status != std::future_status::timeout);
		if (status == std::future_status::ready) {
			return future_to_wait_for.get();
		}
		else {
			throw std::runtime_error("Timeout reached");
		}
	};
}
#endif