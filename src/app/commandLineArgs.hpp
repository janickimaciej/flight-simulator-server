#pragma once

#include "common/mapName.hpp"

namespace App
{
	struct CommandLineArgs
	{
		Common::MapName mapName{};
		int networkThreadPort{};
		int physicsThreadPort{};

		static bool parse(int argc, char** argv, CommandLineArgs& args);
	};
}
