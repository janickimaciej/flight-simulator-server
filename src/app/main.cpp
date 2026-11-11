#include "app/commandLineArgs.hpp"
#include "app/exitCode.hpp"
#include "app/exitSignal.hpp"
#include "app/threads/networkThread.hpp"

#include <memory>

int main(int argc, char** argv)
{
	using namespace App;

	CommandLineArgs args{};
	if (!CommandLineArgs::parse(argc, argv, args))
	{
		return toInt(ExitCode::badArgs);
	}

	ExitSignal exitSignal{};
	auto networkThread = std::make_unique<NetworkThread>(exitSignal, args);
	networkThread->run();

	return toInt(exitSignal.getExitCode());
};
