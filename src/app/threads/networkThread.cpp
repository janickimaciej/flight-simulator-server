#include "app/threads/networkThread.hpp"

#include "app/threads/physicsThread.hpp"
#include "app/udp/udpFrameType.hpp"
#include "physics/airplaneDefinitions.hpp"
#include "physics/playerInfo.hpp"
#include "physics/playerState.hpp"

#include <optional>
#include <vector>

namespace App
{
	NetworkThread::NetworkThread(ExitSignal& exitSignal, const CommandLineArgs& args) :
		m_exitSignal{exitSignal},
		m_simulationBuffer{-1, args.map},
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
		while (!m_exitSignal.exiting())
		{
			kickPlayers();

			asio::ip::udp::endpoint endpoint{};
			Physics::Timestamp clientTimestamp{};
			UDPFrameType udpFrameType{};
			Common::AirplaneType airplaneType{};
			Physics::SimulationStep simulationStep{};
			int playerId{};
			Physics::PlayerInput playerInput{};

			bool received = m_udpCommunication.receiveInitReqOrControlFrame(endpoint,
				clientTimestamp, udpFrameType, airplaneType, simulationStep, playerId, playerInput);

			static const Physics::SimulationStep frameAgeCutoffOffset{0,
				static_cast<unsigned int>(Physics::SimulationStep::stepsPerSecond * 0.9f)};
			Physics::SimulationStep frameAgeCutoff =
				m_simulationClock.getTime() - frameAgeCutoffOffset;
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
			else if (udpFrameType == UDPFrameType::control && simulationStep > m_frameCutoff)
			{
				handleControlFrame(clientTimestamp, simulationStep, playerId, playerInput);
			}
		}
	}

	void NetworkThread::kickPlayers()
	{
		Physics::SimulationStep simulationStep = m_simulationClock.getTime();
		std::vector<int> kickedPlayers = m_playerManager.kickPlayers(simulationStep);
		if (!kickedPlayers.empty())
		{
			m_simulationBuffer.kickPlayers(kickedPlayers, simulationStep);
			m_notification.setNotification(simulationStep, false);
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
			Physics::SimulationStep simulationStep = m_simulationClock.getTime();
			playerId = m_playerManager.addNewPlayer(endpoint, simulationStep);
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
				m_simulationBuffer.writeInitFrame(simulationStep, *playerId, playerInfo);
				m_notification.setNotification(simulationStep, false);
				m_udpCommunication.sendInitResFrame(endpoint, clientTimestamp, *playerId);
			}
		}
	}

	void NetworkThread::handleControlFrame(const Physics::Timestamp& clientTimestamp,
		const Physics::SimulationStep& simulationStep, int playerId,
		const Physics::PlayerInput& playerInput)
	{
		if (!m_playerManager.isPlayerIdValid(playerId))
		{
			return;
		}
		m_simulationBuffer.writeControlFrame(simulationStep, playerId, playerInput);
				m_notification.setNotification(simulationStep, false);
		m_udpCommunication.broadcastControlFrame(m_playerManager.getPlayers(),
			clientTimestamp, simulationStep, playerId, playerInput);
		m_playerManager.bumpPlayer(playerId, m_simulationClock.getTime());
	}
}
