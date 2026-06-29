#include "app/commandLineArg.hpp"

namespace App
{
	std::size_t toSizeT(CommandLineArg commandLineArg)
	{
		return static_cast<std::size_t>(commandLineArg);
	}
}
