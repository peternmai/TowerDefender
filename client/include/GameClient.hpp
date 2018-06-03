#include <iostream>
#include <string>
#include <memory>

#include "rpc/client.h"
#include "rpcMessages.hpp"

#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext.hpp>

class GameClient
{
private:

    std::unique_ptr<rpc::client> client;
    bool validPlayerSession = false;
    uint32_t playerID;



public:
    GameClient(std::string ipAddress, int portNumber);
    ~GameClient();

    uint32_t registerNewPlayerSession(const rpcmsg::PlayerData & playerData);
    bool updatePlayerData(const rpcmsg::PlayerData & playerData);
    rpcmsg::GameData syncGameState();
    rpc::client::connection_state getConnectionState();


};

