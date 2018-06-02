#include "GameEngine.hpp"

#include <LibOVR/OVR_CAPI.h>
#include <LibOVR/OVR_CAPI_GL.h>

#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext.hpp>

#include <iostream>



GameEngine::GameEngine()
{
	// Determine how long program should sleep between update
	this->sleepDuration = std::chrono::nanoseconds(NANOSECONDS_IN_SECOND / REFRESH_RATE);

	// Launch new thread to update program with the given refresh rate
	this->gameEngineServiceStatus = true;
	this->lastUpdateTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch());
	std::thread gameEngineService(&GameEngine::updateService, this);
	gameEngineService.detach();

	std::cout << "\tSuccessfully started game engine service" << std::endl;
}


GameEngine::~GameEngine()
{
	this->gameEngineServiceStatus = false;
}

// Update the game state using the specified refresh rate.
// Note: This should be launched as a new thread
void GameEngine::updateService() 
{
	while (this->gameEngineServiceStatus) {

		// Run update procedure and calculate the compute time
		auto start = std::chrono::high_resolution_clock::now();
		updateProcedure();
		auto end = std::chrono::high_resolution_clock::now();

		// Sleep until the next desired refresh time based on refresh rate
		std::chrono::nanoseconds computeDuration = 
			std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
		std::this_thread::sleep_for(this->sleepDuration - computeDuration);
	}
}

void GameEngine::updateProcedure() 
{
	/**
	auto end = std::chrono::system_clock::now();
	std::chrono::nanoseconds diff = std::chrono::duration_cast<
		std::chrono::nanoseconds>(end - start);
	start = end;
	std::cout << diff.count() << " ns\t|\tRefresh Rate: " << (NANOSECONDS_IN_SECOND / diff.count()) << std::endl;
	*/

	this->updatePlayerData();
	this->updateArrowData();
	this->updateCastleCrasher();
}

// Calculate the new velocity based on the current time
glm::vec3 GameEngine::calculateProjectileVelocity(
	const glm::vec3 & initVelocity, const uint64_t & launchTime)
{
	uint64_t currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::system_clock::now().time_since_epoch()).count();
	float timeDiff = (float)(currentTime - launchTime) / (float) MILLISECONDS_IN_SECOND;

	return (initVelocity + GRAVITY * glm::vec3(0.0f, timeDiff, 0.0f));
}

// Calculate the new position based on the current time
glm::vec3 GameEngine::calculateProjectilePosition(const glm::vec3 & initVelocity,
	const glm::vec3 & initPosition, const uint64_t & launchTime)
{
	uint64_t currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::system_clock::now().time_since_epoch()).count();
	float timeDiff = (float) (currentTime - launchTime) / (float) MILLISECONDS_IN_SECOND;

	return (initPosition + (initVelocity * (timeDiff)) + (0.5f * GRAVITY * glm::vec3(0.0f, std::pow(timeDiff, 2), 0.0f)));
}

void GameEngine::updatePlayerData()
{
	// Get a copy of the previous player data and update it based on new user input
	std::unordered_map<uint32_t, rpcmsg::PlayerData> newPlayerData = this->newPlayerData;
	std::unordered_map<uint32_t, rpcmsg::PlayerData> updatedPlayerData = this->gameData.playerData;

	// SYNCING: If new player appear, add new player
	for (auto player = newPlayerData.begin(); player != newPlayerData.end(); player++)
		if (updatedPlayerData.find(player->first) == updatedPlayerData.end())
			updatedPlayerData[player->first] = newPlayerData[player->first];

	// SYNCING: If a player disconnected, remove player
	for (auto player = updatedPlayerData.begin(); player != updatedPlayerData.end();) {
		if (newPlayerData.find(player->first) == newPlayerData.end())
			player = updatedPlayerData.erase(player);
		else
			player++;
	}

	// Note: Still old data. Not updated yet. Just has same keys as new player data
	this->gameData.playerData = updatedPlayerData;

	// Update the state of the game based on the newly received user input
	for (auto player = newPlayerData.begin(); player != newPlayerData.end(); player++) {

		uint32_t playerID = player->first;

		// Update the user's dominant hand
		if (newPlayerData[playerID].handData[LEFT_HAND].buttonState & ovrButton::ovrButton_X)
			updatedPlayerData[playerID].dominantHand = LEFT_HAND;
		if (newPlayerData[playerID].handData[RIGHT_HAND].buttonState & ovrButton::ovrButton_A)
			updatedPlayerData[playerID].dominantHand = RIGHT_HAND;
		
		uint32_t playerDominantHand = updatedPlayerData[playerID].dominantHand;
		uint32_t playerNonDominantHand = (playerDominantHand == LEFT_HAND) ? RIGHT_HAND : LEFT_HAND;
		glm::vec3 arrowReadyUpZone = glm::vec3(rpcmsg::rpcToGLM(newPlayerData[playerID].handData[playerNonDominantHand].handPose) * glm::vec4(0.0f, 0.0f, ARROW_READY_ZONE_Z_OFFSET, 0.0f));
		glm::vec3 arrowReloadZone = glm::vec3(rpcmsg::rpcToGLM(newPlayerData[playerID].headData.headPose) * glm::vec4(0.0f, 0.0f, ARROW_RELOAD_ZONE_Z_OFFSET, 0.0f));
		glm::vec3 dominantHandPosition = glm::vec3(rpcmsg::rpcToGLM(newPlayerData[playerID].handData[playerDominantHand].handPose) * glm::vec4(0.0f));

		// Player released arrow (arrowReleased == true)
		if (newPlayerData[playerID].arrowReleased == true) {

			// See if player's arrow landed and user can pick up another arrow
			glm::vec3 arrowPosition = glm::vec3(rpcmsg::rpcToGLM(newPlayerData[playerID].arrowData.arrowPose) * glm::vec4(0.0f));
			if (arrowPosition.y < 0.0f) {

				// See if user is reaching for a new arrow
				if (this->gameData.playerData[playerID].handData[playerDominantHand].handTriggerValue < 0.5f)
					if (newPlayerData[playerID].handData[playerDominantHand].handTriggerValue > 0.5f)
						if (glm::length(arrowReloadZone - dominantHandPosition) < ARROW_RELOAD_ZONE_RADIUS)
							updatedPlayerData[playerID].arrowReleased = false;
			}
		}

		// Player is currently holding the arrow (arrowReleased == false && holding)
		else if (newPlayerData[playerID].handData[playerDominantHand].handTriggerValue < 0.5f) {

			// Update the pose of the arrow
			glm::mat4 arrowPose = glm::lookAt(dominantHandPosition, arrowReloadZone, glm::vec3(0.0f, 1.0f, 0.0f));
			arrowPose = glm::translate(glm::mat4(1.0f), dominantHandPosition) * arrowPose;
			updatedPlayerData[playerID].arrowData.arrowPose = rpcmsg::glmToRPC(arrowPose);

			// Player is ready to shoot
			if (newPlayerData[playerID].arrowReadying == true) {

				// Check if user is releasing arrow
				if (newPlayerData[playerID].handData[playerDominantHand].indexTriggerValue < 0.5f) {

					// Store new variables for projectile calculation
					updatedPlayerData[playerID].arrowData.initPosition = rpcmsg::glmToRPC(dominantHandPosition);
					updatedPlayerData[playerID].arrowData.initVelocity = rpcmsg::glmToRPC((arrowReloadZone - dominantHandPosition) * ARROW_VELOCITY_SCALE);

					updatedPlayerData[playerID].arrowReleased = true;
					updatedPlayerData[playerID].arrowReadying = false;
				}
			}

			// Player is not readying to shoot
			else {

				// Check if user is readying up to shoot
				if (glm::length(arrowReadyUpZone - dominantHandPosition) < ARROW_READY_ZONE_RADIUS)
					if (newPlayerData[playerID].handData[playerDominantHand].indexTriggerValue > 0.5f)
						if (this->gameData.playerData[playerID].handData[playerDominantHand].indexTriggerValue < 0.5f)
							updatedPlayerData[playerID].arrowReadying = true;
			}
		}

		// Player dropped the arrow (arrowReleased == false && !holding)
		else {
			updatedPlayerData[playerID].arrowData.arrowPose = rpcmsg::glmToRPC(glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f)));
			updatedPlayerData[playerID].arrowReleased = true;
			updatedPlayerData[playerID].arrowReadying = false;
		}

		// Update hand poses and button pressed
		updatedPlayerData[playerID].headData = newPlayerData[playerID].headData;
		updatedPlayerData[playerID].handData = newPlayerData[playerID].handData;
	}

	// Keep track of all the user's previous player data
	this->gameData.playerData = updatedPlayerData;

}

void GameEngine::updateArrowData() {

	for (auto player = this->gameData.playerData.begin(); player != this->gameData.playerData.begin(); player++) {
		
		uint32_t playerID = player->first;
		glm::vec3 arrowPosition = rpcmsg::rpcToGLM(this->gameData.playerData[playerID].arrowData.arrowPose) * glm::vec4(0.0f);

		// Update arrow projectile if arrow is in the air
		if (this->gameData.playerData[playerID].arrowReleased && (arrowPosition.y < 0.0f)) {

			glm::vec3 initArrowPosition = rpcmsg::rpcToGLM(this->gameData.playerData[playerID].arrowData.initPosition);
			glm::vec3 initArrowVelocity = rpcmsg::rpcToGLM(this->gameData.playerData[playerID].arrowData.initVelocity);
			uint64_t  initArrowLaunchTime = this->gameData.playerData[playerID].arrowData.launchTimeMilliseconds;

			glm::vec3 arrowPosition = this->calculateProjectilePosition(initArrowVelocity, initArrowPosition, initArrowLaunchTime);
			glm::vec3 arrowVelocity = this->calculateProjectileVelocity(initArrowVelocity, initArrowLaunchTime);

			glm::mat4 arrowPose = glm::lookAt(glm::vec3(0.0f), arrowVelocity, glm::vec3(0.0f, 1.0f, 0.0f));
			arrowPose = glm::translate(glm::mat4(1.0f), arrowPosition) * arrowPose;
			this->gameData.playerData[playerID].arrowData.arrowPose = rpcmsg::glmToRPC(arrowPose);
		}
	}
}

void GameEngine::updateCastleCrasher() {

	// Determine if arrows hit any of the castle crashers
	for (auto player = this->gameData.playerData.begin(); player != this->gameData.playerData.begin(); player++) {

		uint32_t playerID = player->first;
		glm::vec3 arrowPosition = rpcmsg::rpcToGLM(this->gameData.playerData[playerID].arrowData.arrowPose) * glm::vec4(0.0f);

		if (this->gameData.playerData[playerID].arrowReleased && (arrowPosition.y < 0.0f)) {
			for (auto castleCrasher = this->gameData.gameState.castleCrasherData.begin(); castleCrasher != this->gameData.gameState.castleCrasherData.end(); castleCrasher++) {
				if (castleCrasher->alive) {
					glm::vec3 castleCrasherPosition = rpcmsg::rpcToGLM(castleCrasher->position);
					if (glm::length(arrowPosition - castleCrasherPosition) < CASTLE_CRASHER_HIT_RADIUS) {
						castleCrasher->health = std::max(castleCrasher->health - (float) ARROW_DAMAGE, 0.0f);
						castleCrasher->alive = (castleCrasher->health == 0.0f) ? false : true;
						this->gameData.playerData[playerID].arrowData.arrowPose = rpcmsg::glmToRPC(glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f)));
						break;
					}
				}
			}
		}
	}

	// Update the position of each of the castle crasher
	for (auto castleCrasher = this->gameData.gameState.castleCrasherData.begin(); castleCrasher != this->gameData.gameState.castleCrasherData.end(); castleCrasher++) {
		if (castleCrasher->alive) {

		}
	}


}

void GameEngine::handleNewUserInput(uint32_t playerID, const rpcmsg::PlayerData & newInputs) {
	this->newPlayerData[playerID] = newInputs;
}

void GameEngine::removeUser(uint32_t playerID) {
	this->newPlayerData.erase(playerID);
}