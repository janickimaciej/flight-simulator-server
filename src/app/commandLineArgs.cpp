#include "app/commandLineArgs.hpp"

#include "app/commandLineArg.hpp"

#include <string>

namespace App
{
	bool CommandLineArgs::parse(int argc, char** argv, CommandLineArgs& args)
	{
		if (argc != argCount)
		{
			return false;
		}

		int mapIndex = std::stoi(argv[toSizeT(CommandLineArg::map)]);
		if (mapIndex < 0 || mapIndex >= Common::mapCount)
		{
			return false;
		}
		args.map = static_cast<Common::MapName>(mapIndex);

		static constexpr int minPortValue = 0;
		static constexpr int maxPortValue = 1 << 16;

		args.networkThreadPort = std::stoi(argv[toSizeT(CommandLineArg::networkThreadPort)]);
		if (args.networkThreadPort < minPortValue || args.networkThreadPort >= maxPortValue)
		{
			return false;
		}

		args.physicsThreadPort = std::stoi(argv[toSizeT(CommandLineArg::physicsThreadPort)]);
		if (args.physicsThreadPort < minPortValue || args.physicsThreadPort >= maxPortValue)
		{
			return false;
		}

		return true;
	}
}
