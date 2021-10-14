#include <vector>
#include <string>
#include <random>

#include <gtest/gtest.h>

#include "../include/static_dispatcher.hpp"

TEST(Dispatch, static_dispatcher_test)
{
	constexpr size_t messageCount = 4096;

	// Generate input messages
	std::random_device randomDevice;
	std::mt19937 randomEngine(randomDevice());
	std::uniform_int_distribution<int> distribution('a', 'z');

	std::vector<std::vector<std::byte>> inputData;

	std::vector<std::byte> randomData(32);
	for (size_t i = 0; i < messageCount; ++i)
	{
		std::ranges::generate(randomData, [&]() { return static_cast<std::byte>(distribution(randomEngine)); });
		inputData.push_back(randomData);
	}

	// Test recorder
	struct
	{
		void save_message(std::string_view message) { _messages.emplace_back(message); }
		std::vector<std::string> _messages;
	} test_recorder;

	// Create dispatcher
	boost::asio::thread_pool threadPool;

	{
		auto dispatcher = demo::static_dispatcher(threadPool.get_executor(), test_recorder);

		// Process messages
		for (auto & message : inputData)
			dispatcher.dispatch(message);

		// Wait till all messages are processed
	}

	// Check recorded message count
	EXPECT_EQ(std::size(inputData), std::size(test_recorder._messages));

	for (size_t i = 0; i < messageCount; ++i)
	{
		// Check message size (it should be greater than input message)
		EXPECT_LT(std::size(inputData[i]), std::size(test_recorder._messages[i]));

		// Compare data
		EXPECT_EQ(memcmp(std::data(inputData[i]), std::data(test_recorder._messages[i]), std::size(inputData[i])), 0);
	}
}
