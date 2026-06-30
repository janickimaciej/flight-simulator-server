#include "app/threads/networkThread.hpp"

#include "app/threads/physicsThread.hpp"
#include "app/udp/udpFrameType.hpp"
#include "common/terrains/maps/maps.hpp"
#include "physics/airplaneDefinitions.hpp"
#include "physics/playerInfo.hpp"

#include <optional>
#include <vector>

namespace App
{
	NetworkThread::NetworkThread(ExitSignal& exitSignal, const CommandLineArgs& args) :
		m_exitSignal{exitSignal},
		m_simulationBuffer{-1, args.map},
		m_spawner{*Common::Terrains::maps[toSizeT(args.map)]},
		m_udpCommunication{args.networkThreadPort, args.physicsThreadPort}
	{ }

	void NetworkThread::start()
	{
		m_frameCutoff = m_simulationClock.getTime();
		PhysicsThread physicsThread{m_exitSignal, m_simulationClock, m_simulationBuffer,
			m_notification, m_udpCommunication, m_playerManager};
		mainLoop();
		physicsThread.join();
	}

	void NetworkThread::mainLoop()
	{
		while (!m_exitSignal.shouldStop())
		{
			kickPlayers();

			asio::ip::udp::endpoint endpoint{};
			Physics::Timestamp clientTimestamp{};
			UDPFrameType udpFrameType{};
			Common::AirplaneType airplaneType{};
			Physics::Timestep timestep{};
			int playerId{};
			Physics::PlayerInput playerInput{};

			bool received = m_udpCommunication.receiveInitReqOrControlFrame(endpoint,
				clientTimestamp, udpFrameType, airplaneType, timestep, playerId, playerInput);

			static constexpr Physics::Timestep frameAgeCutoffOffset{0,
				static_cast<unsigned int>(Common::stepsPerSecond * 0.9f)};
			Physics::Timestep frameAgeCutoff = m_simulationClock.getTime() - frameAgeCutoffOffset;
			if (m_frameCutoff < frameAgeCutoff)
			{
				m_frameCutoff = frameAgeCutoff;
			}

			if (!received)
			{
				continue;
			}

			if (udpFrameType == UDPFrameType::initReq)
			{
				handleInitReqFrame(endpoint, clientTimestamp, airplaneType);
			}
			else if (udpFrameType == UDPFrameType::control && timestep > m_frameCutoff)
			{
				handleControlFrame(clientTimestamp, timestep, playerId, playerInput);
			}
		}
	}

	void NetworkThread::kickPlayers()
	{
		Physics::Timestep timestep = m_simulationClock.getTime();
		std::vector<int> kickedPlayers = m_playerManager.kickPlayers(timestep);
		if (!kickedPlayers.empty())
		{
			m_simulationBuffer.kickPlayers(kickedPlayers, timestep);
			m_notification.setNotification(timestep, false);
		}
	}

	void NetworkThread::handleInitReqFrame(const asio::ip::udp::endpoint& endpoint,
		const Physics::Timestamp& clientTimestamp, const Common::AirplaneType& airplaneType)
	{
		std::optional<int> playerId = m_playerManager.getPlayerId(endpoint);
		if (playerId)
		{
			m_udpCommunication.sendInitResFrame(endpoint, clientTimestamp, *playerId);
		}
		else
		{
			Physics::Timestep timestep = m_simulationClock.getTime();
			playerId = m_playerManager.addNewPlayer(endpoint, timestep);
			if (playerId)
			{
				Physics::PlayerInfo playerInfo
				{
					Physics::PlayerInput{},
					Physics::PlayerState
					{
						airplaneType,
						Physics::airplaneDefinitions[Common::toSizeT(airplaneType)].initialHP,
						m_spawner.spawn(airplaneType)
					}
				};
				m_simulationBuffer.writeInitFrame(timestep, *playerId, playerInfo);
				m_notification.setNotification(timestep, false);
				m_udpCommunication.sendInitResFrame(endpoint, clientTimestamp, *playerId);
			}
		}
	}

	void NetworkThread::handleControlFrame(const Physics::Timestamp& clientTimestamp,
		const Physics::Timestep& timestep, int playerId, const Physics::PlayerInput& playerInput)
	{
		if (!m_playerManager.isPlayerIdValid(playerId))
		{
			return;
		}
		m_simulationBuffer.writeControlFrame(timestep, playerId, playerInput);
				m_notification.setNotification(timestep, false);
		m_udpCommunication.broadcastControlFrame(m_playerManager.getPlayers(),
			clientTimestamp, timestep, playerId, playerInput);
		m_playerManager.bumpPlayer(playerId, m_simulationClock.getTime());
	}
}
