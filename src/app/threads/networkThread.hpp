#pragma once

#include "app/commandLineArgs.hpp"
#include "app/exitSignal.hpp"
#include "app/playerManager.hpp"
#include "app/udp/udpCommunication.hpp"
#include "common/airplaneType.hpp"
#include "physics/notification.hpp"
#include "physics/playerInput.hpp"
#include "physics/simulationBuffer.hpp"
#include "physics/simulationClock.hpp"
#include "physics/spawner.hpp"
#include "physics/timestamp.hpp"
#include "physics/timestep.hpp"

#include <asio/asio.hpp>

namespace App
{
	class NetworkThread
	{
	public:
		NetworkThread(ExitSignal& exitSignal, const CommandLineArgs& args);
		void start();

	private:
		ExitSignal& m_exitSignal;

		Physics::SimulationClock m_simulationClock{};
		Physics::SimulationBuffer m_simulationBuffer;
		Physics::Spawner m_spawner;

		Physics::Notification m_notification{m_simulationClock};
		Physics::Timestep m_frameCutoff{};
		UDPCommunication m_udpCommunication;

		PlayerManager m_playerManager{};

		void mainLoop();

		void kickPlayers();
		void handleInitReqFrame(const asio::ip::udp::endpoint& endpoint,
			const Physics::Timestamp& clientTimestamp, const Common::AirplaneType& airplaneType);
		void handleControlFrame(const Physics::Timestamp& clientTimestamp,
			const Physics::Timestep& timestep, int playerId,
			const Physics::PlayerInput& playerInput);
	};
}
