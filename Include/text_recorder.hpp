#include <fstream>
#include <filesystem>

namespace demo
{

	class text_recorder
	{
	public:
		text_recorder(text_recorder &) = delete;

		text_recorder(text_recorder &&) = default;

		text_recorder(const std::filesystem::path & path) :
			_outputFileStream(path, std::ios::out)
		{
			if (_outputFileStream.fail())	// Considering fatal
				throw std::runtime_error("Failed to open file");
		}

		void save_message(std::string_view message)
		{
			_outputFileStream << message << std::endl;
		}

	private:
		std::ofstream _outputFileStream;
	};

}