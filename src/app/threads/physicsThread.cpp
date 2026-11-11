#include "app/threads/physicsThread.hpp"

#include "physics/playerInfo.hpp"

#include <unordered_map>
#include <utility>

namespace App
{
	PhysicsThread::PhysicsThread(ExitSignal& exitSignal,
		const Physics::SimulationClock& simulationClock,
		Physics::SimulationBuffer& simulationBuffer, Physics::Notification& notification,
		UDPCommunication& udpCommunication, PlayerManager& playerManager) :
		m_exitSignal{exitSignal},
		m_simulationClock{simulationClock},
		m_simulationBuffer{simulationBuffer},
		m_notification{notification},
		m_udpCommunication{udpCommunication},
		m_playerManager{playerManager},
		m_thread
		{
			[this]
			{
				this->run();
			}
		}
	{ }

	void PhysicsThread::join()
	{
		m_thread.join();
	}

	void PhysicsThread::run()
	{
		Physics::SimulationStep initialSimulationStep = m_simulationClock.getTime();

		m_simulationBuffer.update(initialSimulationStep);

		mainLoop(initialSimulationStep);
	}

	void PhysicsThread::mainLoop(const Physics::SimulationStep& initialSimulationStep)
	{
		Physics::SimulationStep simulationStep = initialSimulationStep;
		while (!m_exitSignal.exiting())
		{
			simulationStep = simulationStep.next();
			m_notification.getNotification(simulationStep);
			sleepIfFuture(simulationStep);

			m_simulationBuffer.update(simulationStep);

			if (simulationStep.getStep() == 0)
			{
				std::unordered_map<int, Physics::PlayerInfo> playerInfos =
					m_simulationBuffer.getPlayerInfos(simulationStep);
				for (const std::pair<const int, Physics::PlayerInfo>& playerInfo : playerInfos)
				{
					if (playerInfo.second.state.hp == 0)
					{
						m_playerManager.killPlayer(playerInfo.first, simulationStep);
					}
				}
				m_udpCommunication.broadcastStateFrame(m_playerManager.getPlayers(), simulationStep,
					playerInfos);
			}
		}
	}

	void PhysicsThread::sleepIfFuture(const Physics::SimulationStep& simulationStep)
	{
		Physics::SimulationStep currSimulationStep = m_simulationClock.getTime();
		while (currSimulationStep < simulationStep)
		{
			currSimulationStep = m_simulationClock.getTime();
		}
	}
}
