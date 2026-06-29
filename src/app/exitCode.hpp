#pragma once

namespace App
{
	enum class ExitCode
	{
		ok,
		badArgs
	};

	int toInt(ExitCode exitCode);
}
