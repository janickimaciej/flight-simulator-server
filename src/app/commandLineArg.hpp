#pragma once

#include <cstddef>

namespace App
{
	inline constexpr int argCount = 4;

	enum class CommandLineArg
	{
		programName,
		map,
		networkThreadPort,
		physicsThreadPort
	};

	std::size_t toSizeT(CommandLineArg commandLineArg);
}
