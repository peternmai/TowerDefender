#include "GameClient.hpp"

#include <chrono>
#include <thread>

uint32_t GameClient::registerNewPlayerSession(const rpcmsg::PlayerData & playerData) {
    try {
        this->playerID = this->client->call(rpcmsg::REQUEST_SERVER_SESSION, playerData).as<uint32_t>();
        this->validPlayerSession = true;
    }
    catch (const std::exception&) {
        std::cerr << "Unable to register new player session" << std::endl;
        this->playerID = 0;
    }

    return this->playerID;
}

bool GameClient::updatePlayerData(const rpcmsg::PlayerData & playerData) {
    try {
        this->client->call(rpcmsg::UPDATE_PLAYER_DATA, this->playerID, playerData);
        return true;
    }
    catch (const std::exception&) {
        std::cerr << "Unable to update user's data" << std::endl;
        return false;
    }
}


rpcmsg::GameData GameClient::syncGameState() {
    std::vector<char> raw_data = this->client->call(rpcmsg::GET_GAME_DATA).as<std::vector<char>>();
    
    RPCLIB_MSGPACK::object_handle oh = RPCLIB_MSGPACK::unpack(raw_data.data(), raw_data.size());
    RPCLIB_MSGPACK::object obj = oh.get();
    //std::cout << raw_data.size() << std::endl;
    //std::cout << obj << std::endl;

    rpcmsg::GameData gameData;
    obj.convert(gameData);

    return gameData;
}

GameClient::GameClient(std::string ipAddress, int portNumber)
{
    // Attempt to connect to the specified server
    std::cout << "Attempting to connect to " << ipAddress << ":" << portNumber << "..." << std::endl;
    this->client = std::make_unique<rpc::client>(ipAddress, portNumber);
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    rpc::client::connection_state cs = client->get_connection_state();
    
    // While connection fails, attempt to reconnect
    while (cs != rpc::client::connection_state::connected) {
        std::cerr << "\tUnable to connect. Retrying..." << std::endl;
        this->client.reset();
        this->client = std::make_unique<rpc::client>(ipAddress, portNumber);
        std::this_thread::sleep_for(std::chrono::milliseconds(5000));
        cs = client->get_connection_state();
    }

    std::cout << "\tSuccessfully connected to server." << std::endl;
}

rpc::client::connection_state GameClient::getConnectionState() {
    return this->client->get_connection_state();
}


GameClient::~GameClient()
{
    if (this->validPlayerSession)
        this->client->call(rpcmsg::CLOSE_SERVER_SESSION, this->playerID);
}