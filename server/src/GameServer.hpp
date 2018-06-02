#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <mutex>

#include "rpc/server.h"
#include "rpcMessages.hpp"
#include "GameEngine.hpp"

#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext.hpp>

#define LEFT_HAND  0
#define RIGHT_HAND 1

#define MAX_PLAYER 2

#define NUM_WORKER 10

class GameServer
{
private:

	std::unique_ptr<GameEngine> gameEngine;
	std::unique_ptr<rpc::server> server;
	std::mutex requestSessionLock;

	// Store information about the entire game state
	rpcmsg::GameData gameData;

	// Remote Procedure Calls
	void updatePlayerData(uint32_t playerID, rpcmsg::PlayerData const & playerData);
	std::vector<char> getEntireGameData();
	uint32_t requestServerSession();
	void closeServerSession(uint32_t playerID);

public:

	GameServer(int portNumber);
	~GameServer();
	void stop();
	
};

