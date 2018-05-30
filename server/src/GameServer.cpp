#include "GameServer.hpp"
#include "rpc/this_handler.h"

#include <cstdlib>
#include <ctime>

bool DEBUG = true;

// Client passes an update of their user data (pose of head and hands)
void GameServer::updatePlayerData(uint32_t playerID, rpcmsg::PlayerData const & playerData) {

	// Check that the user id the user specified is valid
	if (this->gameData.playerData.find(playerID) == this->gameData.playerData.end())
		rpc::this_handler().respond_error(rpcmsg::INVALID_USER);
	
	// All is good, update the server's data
	this->gameData.playerData[playerID] = playerData;
}

// Client wants to get a copy of the current state of the game.
// TODO: Need better way to send custom struct to client from server
std::vector<char> GameServer::getEntireGameData() {
	RPCLIB_MSGPACK::sbuffer buffer;
	RPCLIB_MSGPACK::pack(buffer, this->gameData);

	std::vector<char> raw_data;
	raw_data.resize(buffer.size());
	std::memcpy(raw_data.data(), buffer.data(), buffer.size());
	return raw_data;
}

// Client wants to join the game. Return a player ID if max user has not exceeded.
uint32_t GameServer::requestServerSession() {
	if (DEBUG) std::cout << "Client requestiong game session..." << std::endl;

	// Check if user can join
	this->requestSessionLock.lock();
	if (this->gameData.playerData.size() >= MAX_PLAYER) {
		this->requestSessionLock.unlock();
		if (DEBUG) std::cout << std::endl << "\tCannot register anymore players!" << std::endl;
		rpc::this_handler().respond_error(rpcmsg::MAX_USER_EXCEEDED);
	}

	// Create a new session for the user
	uint32_t playerID;
	srand((unsigned int)time(NULL));
	do {
		playerID = rand() % std::numeric_limits<uint32_t>::max();
	} while (this->gameData.playerData.find(playerID) != this->gameData.playerData.end());
	rpcmsg::PlayerData newPlayerData{};
	this->gameData.playerData[playerID] = newPlayerData;

	// Return the player's ID number
	if (DEBUG) std::cout << "\tNew player ID " << playerID << " registered" << std::endl;
	this->requestSessionLock.unlock();
	return playerID;
}

// Client has exited the game. Close the client's session
void GameServer::closeServerSession(uint32_t playerID) {
	if (DEBUG) std::cout << "Ending session for player " << playerID << std::endl;
	this->gameData.playerData.erase(playerID);
}

GameServer::GameServer(int portNumber)
{
	// Instantiate a new server object
	this->server = std::make_unique<rpc::server>(portNumber);

	// Bind function to update server's player data
	this->server->bind(rpcmsg::UPDATE_PLAYER_DATA, 
		[this](uint32_t playerID, rpcmsg::PlayerData const & playerData) {
		this->updatePlayerData(playerID, playerData); });

	// Bind function to return server's game state to client
	this->server->bind(rpcmsg::GET_GAME_DATA, [this]() {
		return this->getEntireGameData();
	});

	// Bind function to allow client to join the game
	this->server->bind(rpcmsg::REQUEST_SERVER_SESSION, [this]() {
		return this->requestServerSession(); });

	// Bind function to allow client to leave game session
	this->server->bind(rpcmsg::CLOSE_SERVER_SESSION, [this](uint32_t playerID) {
		this->closeServerSession(playerID); });

	// Spawn multiple worker to handle client requests
	this->server->async_run(NUM_WORKER);
	std::cout << "\tServer is up and running" << std::endl;
}


GameServer::~GameServer()
{
}

void GameServer::stop() {
	this->server->close_sessions();
	this->server->stop();
	this->server.reset();
}