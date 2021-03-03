#pragma once
#include "Game.h"
#include "../Engine/NetworkObject.h"

namespace NCL {
	namespace CSC8508 {
		class GameServer;
		class GameClient;
		class NetworkPlayer;

		static class LocalHost {
			public:
				static bool IsLocalHostMode() { return true; }
				static int GetA() { return 192; }
				static int GetB() { return 168; }
				static int GetC() { return 0; }
				static int GetD() { return 15; }
				static int GetPort() { return 80; }


		};

		class NetworkedGame : public Game, public PacketReceiver {
		public:
			NetworkedGame();
			~NetworkedGame();

			void StartAsServer();
			void StartAsClient(char a, char b, char c, char d);

			void UpdateGame(float dt) override;

			void SpawnPlayer();

			void StartLevel();

			void ReceivePacket(int type, GamePacket* payload, int source) override;

			void OnPlayerCollision(NetworkPlayer* a, NetworkPlayer* b);

		protected:
			void UpdateAsServer(float dt);
			void UpdateAsClient(float dt);

			void BroadcastSnapshot(bool deltaFrame);
			void UpdateMinimumState();
			std::map<int, int> stateIDs;

			GameServer* thisServer;
			GameClient* thisClient;
			float timeToNextPacket;
			int packetsToSnapshot;

			std::vector<NetworkObject*> networkObjects;

			std::map<int, GameObject*> serverPlayers;
			GameObject* localPlayer;
		};
	}
}
