#include <unordered_map>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <thread>
#include <chrono>

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

#define TIMEOUT_SECONDS           5
#define MAINTENANCE_TIMER_SECONDS 1


class GameServer
{
private:

    struct CommunicationMetadata {
        uint32_t playerID;
        std::chrono::nanoseconds lastCommunicated;
    };

    // Keeps track of server communication
    std::unordered_map<uint32_t, CommunicationMetadata> communicationMetadata;
    std::unique_ptr<rpc::server> server;
    std::mutex requestSessionLock;
    bool serverActive;

    // Instance of the game engine
    std::unique_ptr<GameEngine> gameEngine;

    // Remote Procedure Calls
    void updatePlayerData(uint32_t playerID, rpcmsg::PlayerData const & playerData);
    std::vector<char> getEntireGameData();
    uint32_t requestServerSession(const rpcmsg::PlayerData & playerData);
    void closeServerSession(uint32_t playerID);

    // Helper function
    std::chrono::nanoseconds getCurrentTime();
    void serverPeriodicMaintenance();

public:

    GameServer(int portNumber);
    ~GameServer();
    void stop();
    
};

