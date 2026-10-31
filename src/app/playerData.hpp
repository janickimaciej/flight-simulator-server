#pragma once

#include "physics/simulationStep.hpp"

#include <asio/asio.hpp>

namespace App
{
	struct PlayerData
	{
		asio::ip::udp::endpoint endpoint;
		Physics::SimulationStep keepAliveSimulationStep{};
		bool keepAliveLock{};
	};
}
