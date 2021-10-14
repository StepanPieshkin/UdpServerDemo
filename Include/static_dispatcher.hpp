#include <filesystem>
#include <iostream>
#include <fstream>
#include <span>
#include <thread>
#include <mutex>

#include "boost/asio.hpp"

#include <concurrentqueue/concurrentqueue.h>

namespace demo
{

	template<typename T>
	concept recorder =
		requires(T r, std::string_view message) {
			{ r.save_message(message) };
	};

	template<typename Executor, recorder Recorder>
	class static_dispatcher
	{
		struct work_item
		{
			work_item(std::string && message) :
				_stream(std::forward<std::string>(message), std::ios_base::ate)
			{}

			std::ostringstream _stream;
			std::atomic_bool _done;
		};

	public:
		static_dispatcher(static_dispatcher &) = delete;

		static_dispatcher(static_dispatcher &&) = delete;

		static_dispatcher(Executor executor, Recorder & recorder) noexcept :
			_executor(executor), _recorder(recorder), _writerThread([this] { write_messages(); })
		{
		}

		~static_dispatcher()
		{
			// Signal stop was requested
			_mutex.lock();
			_stopRequested = true;
			_condVar.notify_one();
			_mutex.unlock();

			// Wait till all messages are written to the file
			_writerThread.join();
		}

		void dispatch(std::span<std::byte> data)
		{
			// Allocating message and trying to avoid memory reallocation
			std::string message;
			message.reserve(std::size(data) + 44 /*max size of two space separated handles*/);
			message.resize(std::size(data));
			memcpy(std::data(message), std::data(data), std::size(data));

			// Enqueue message
			std::unique_ptr<work_item> item = std::make_unique<work_item>(std::move(message));
			work_item * itemPtr = item.get();
			_messageQueue.enqueue(std::move(item));

			// Enqueue message for processing
			boost::asio::execution::execute(_executor, [itemPtr, this]() mutable
				{
					// Thread local message counter
					static thread_local size_t counter = 0;

					// Add thread id and local count to message
					itemPtr->_stream << " " << std::this_thread::get_id() << " " << counter++;

					// Simulate work
					std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 2));

					// Signal completion
					itemPtr->_done.store(true, std::memory_order::release);
					_condVar.notify_one();
				});
		}

	private:
		// Since there is no convenient cross-platform way to asynchronously write file - just do it synchronously
		void write_messages()
		{
			for (;;)
			{
				// Draining the queue
				std::unique_ptr<work_item> item;
				while (_messageQueue.try_dequeue(item))
				{
					// Ensure message is already processed
					while (!item->_done.load(std::memory_order::acquire))
					{
						std::unique_lock<std::mutex> lock(_mutex);
						_condVar.wait(lock);	// We don't mind the spurious wake ups here
					}

					// Write message to file
					_recorder.save_message(item->_stream.str());
				}

				// Stopping?
				std::unique_lock<std::mutex> lock(_mutex);
				if (_stopRequested)
					break;

				// Wait for new messages and repeat
				_condVar.wait(lock); // We don't mind the spurious wake ups here
			}
		}

		Executor _executor;

		Recorder & _recorder;

		moodycamel::ConcurrentQueue<std::unique_ptr<work_item>> _messageQueue;

		std::thread _writerThread;

		std::mutex _mutex;

		std::condition_variable _condVar;

		bool _stopRequested = false;
	};

}