#include "boost/asio.hpp"

#include "../include/udp_server.hpp"
#include "../include/static_dispatcher.hpp"
#include "../include/text_recorder.hpp"

std::filesystem::path get_self_module_file_name()
{
	std::filesystem::path exePath;

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
	TCHAR fileName[MAX_PATH];
	if (::GetModuleFileName(nullptr, fileName, MAX_PATH) != 0)
		exePath = fileName;
#else
	char fileName[PATH_MAX];
	ssize_t len = ::readlink("/proc/self/exe", fileName, sizeof(fileName) - 1);
	if (len != -1)
	{
		fileName[len] = '\0';
		exePath = fileName;
	}
#endif

	return exePath;
}

std::filesystem::path get_self_module_directory_name()
{
	return get_self_module_file_name().remove_filename();
}

int main(int argc, char * argv[])
{
	// By default server will use 4 IO threads for network operations
	size_t ioThreadCount = 4;

	// Default binding port
	size_t port = 55514;

	// Default binding address
	const char * address = "127.0.0.1";

	// Get command line arguments if any
	if (argc > 1)	// 1st argument
	{
		// Address
		address = argv[1];

		if (argc > 2)	// 2nd argument
		{
			// Port
			size_t value;
			if (std::from_chars(argv[2], argv[2] + strlen(argv[2]), value).ec == std::errc())
				port = value;

			if (argc > 3)	// 3rd argument
			{
				// IO thread count
				if (std::from_chars(argv[3], argv[3] + strlen(argv[3]), value).ec == std::errc())
					ioThreadCount = value;
			}
		}
	}

	// Thread pool with default thread count (equal to number of CPU cores)
	boost::asio::thread_pool threadPool;

	// Create recorder
	demo::text_recorder recorder(std::filesystem::absolute(get_self_module_directory_name() / "output.log"));

	// Create dispatcher
	auto dispatcher = demo::static_dispatcher(threadPool.get_executor(), recorder);

	// Start TCP server
	auto demoServer = demo::udp_server(dispatcher,
		{ boost::asio::ip::make_address_v4(address), static_cast<boost::asio::ip::port_type>(port) },
		ioThreadCount);

	getchar();

	return 0;
}