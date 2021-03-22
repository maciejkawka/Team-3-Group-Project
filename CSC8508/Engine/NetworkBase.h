#pragma once
#include <winsock2.h>
#include <enet/enet.h>
#include <map>
#include <string>

enum BasicNetworkMessages {
	None,
	Hello,
	Message,
	String_Message,
	Delta_State,	//1 byte per channel since the last state
	Full_State,		//Full transform etc
	Player_Delta_State,
	Player_Full_State,
	Received_State, //received from a client, informs that its received packet n
	Player_Connected,
	Player_Disconnected,
	Shutdown,
	Player_Count,
	Player_Finished,
	Exit_Lobby,
	Player_State
};

struct GamePacket {
	short size;
	short type;

	GamePacket() {
		type		= BasicNetworkMessages::None;
		size		= 0;
	}

	GamePacket(short type) : GamePacket() {
		this->type	= type;
	}

	int GetTotalSize() {
		return sizeof(GamePacket) + size;
	}
};

struct StringPacket : public GamePacket {
	char	stringData[256];

	StringPacket(const std::string& message) {
		type		= BasicNetworkMessages::String_Message;
		size		= (short)message.length();

		memcpy(stringData, message.data(), size);
	};

	std::string GetStringFromData() {
		std::string realString(stringData);
		realString.resize(size);
		return realString;
	}
};

struct NewPlayerPacket : public GamePacket {
	int playerID;
	NewPlayerPacket(int p ) {
		type		= BasicNetworkMessages::Player_Connected;
		playerID	= p;
		size		= sizeof(int);
	}
};

struct PlayerDisconnectPacket : public GamePacket {
	int playerID;
	PlayerDisconnectPacket(int p) {
		type		= BasicNetworkMessages::Player_Disconnected;
		playerID	= p;
		size		= sizeof(int);
	}
};

struct PlayerCountPacket : public GamePacket {
	int playerIDs[8];
	PlayerCountPacket(int ids[8]) {
		type = BasicNetworkMessages::Player_Count;
		for (int i = 0; i < 8; i++) playerIDs[i] = ids[i];
		size = sizeof(int) * 8;

	}

};

struct PlayerFinishedPacket : public GamePacket {
	int playerID;
	int score;
	PlayerFinishedPacket(int id, int finalScore) {
		playerID = id;
		score = finalScore;
		type = BasicNetworkMessages::Player_Finished;
		size = sizeof(int) *2;

	}

};

struct PlayerStatePacket : public GamePacket {
	int playerID;
	int score;
	bool isFinished;
	//Animation Coming Soon

	PlayerStatePacket(int id, int score, bool isFinished = false) {
		playerID = id;
		this->score = score;
		this->isFinished = isFinished;
		type = BasicNetworkMessages::Player_State;

	}
};

struct ExitLobbyPacket : public GamePacket {
	ExitLobbyPacket() {
		type = BasicNetworkMessages::Exit_Lobby;
	}

};

class PacketReceiver {
public:
	virtual void ReceivePacket(int type, GamePacket* payload, int source = -1) = 0;
};

class NetworkBase	{
public:
	static void Initialise();
	static void Destroy();

	static int GetDefaultPort() {
		return 80;
	}

	void RegisterPacketHandler(int msgID, PacketReceiver* receiver) {
		packetHandlers.insert(std::make_pair(msgID, receiver));
	}


protected:
	NetworkBase();
	~NetworkBase();

	bool ProcessPacket(GamePacket* p, int peerID = -1);

	typedef std::multimap<int, PacketReceiver*>::const_iterator PacketHandlerIterator;

	bool GetPacketHandlers(int msgID, PacketHandlerIterator& first, PacketHandlerIterator& last) const {
		auto range = packetHandlers.equal_range(msgID);

		if (range.first == packetHandlers.end()) {
			return false; //no handlers for this message type!
		}
		first	= range.first;
		last	= range.second;
		return true;
	}

	ENetHost* netHandle;

	std::multimap<int, PacketReceiver*> packetHandlers;
};