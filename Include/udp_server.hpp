#include <span>
#include <thread>
#include <iostream>

#include "boost/asio.hpp"

namespace demo
{

	template<typename T>
	concept dispatcher =
		requires(T d, std::span<std::byte> data) {
			{ d.dispatch(data) };
	};

	template<dispatcher Dispatcher>
	class udp_server
	{
	public:
		udp_server(udp_server &) = delete;

		udp_server(udp_server &&) = default;

		~udp_server()
		{
			_ioContext.stop();

			for (auto & thread : _ioThreads)
				thread.join();

			std::cout << std::format("UDP server stopped\n");
		}

		udp_server(Dispatcher & dispatcher, boost::asio::ip::udp::endpoint && endpoint, size_t threadCount = 1) :
			_dispatcher(dispatcher), _ioContext((int)threadCount), _signals(_ioContext, SIGINT, SIGTERM), _endpoint(std::forward<boost::asio::ip::udp::endpoint>(endpoint)), _socket(_ioContext, _endpoint)
		{
			std::cout << std::format("Starting UDP server: io thread count {}\n", threadCount);

			_signals.async_wait([&](const boost::system::error_code & error, int signal_number) { std::cout << std::format("UDP server IO error: {}\n", error.message()); });

			_ioThreads.reserve(threadCount);
			for (size_t i = 0; i < threadCount; ++i)
				_ioThreads.emplace_back([this] { _ioContext.run(); });

			for (size_t i = 0; i < threadCount; ++i)
				boost::asio::co_spawn(_ioContext, receive(), boost::asio::detached);
		}

	private:
		boost::asio::awaitable<void> receive() noexcept
		{
			try
			{
				std::array<std::byte, 128> buffer;	// We don't expect anything greater than 128 bytes
				boost::asio::ip::udp::endpoint remoteEndpoint;

				// Receive data as long as context is not stopped
				for (;;)
				{
					size_t bytesReceived = co_await _socket.async_receive_from(boost::asio::buffer(buffer), remoteEndpoint, boost::asio::use_awaitable);

					//std::cout << std::format("Received {} bytes from endpoint {}:{}\n", bytesReceived, remoteEndpoint.address().to_string(), (size_t)remoteEndpoint.port());

					_dispatcher.dispatch(std::ranges::subrange(std::begin(buffer), std::begin(buffer) + bytesReceived));
				}
			}
			catch (std::exception & e)
			{
				std::cout << std::format("Receiver exception: {}\n", e.what());
			}
		}

		Dispatcher & _dispatcher;

		std::vector<std::thread> _ioThreads;

		boost::asio::io_context _ioContext;

		boost::asio::signal_set _signals;

		boost::asio::ip::udp::endpoint _endpoint;

		boost::asio::ip::udp::socket _socket;
	};

}