#include "GameEngine.hpp"

#include <random>

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

    // Initialize game data
    this->gameData.gameState.castleHealth = 100.0f;
    this->gameData.gameState.gameStarted = false;
    this->gameData.gameState.leftTowerReady = false;
    this->gameData.gameState.rightTowerReady = false;

    // Launch new thread to update program with the given refresh rate
    this->gameEngineServiceStatus = true;
    this->lastUpdateTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch());
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

    // Get an instance of the current game state
    this->gameDataLock.lock();
    rpcmsg::GameData updatedGameData = this->gameData;
    this->gameDataLock.unlock();

    // Perform update procedure
    updatedGameData = this->updatePlayerData(updatedGameData);
    updatedGameData = this->updateArrowData(updatedGameData);
    updatedGameData = this->updateCastleCrasher(updatedGameData);
    updatedGameData = this->updateGameState(updatedGameData);
    updatedGameData = this->updateEasterEgg(updatedGameData);

    // Assign the game state
    this->gameDataLock.lock();
    this->gameData = updatedGameData;
    this->gameDataLock.unlock();
}

// Calculate the new velocity based on the current time
glm::vec3 GameEngine::calculateProjectileVelocity(
    const glm::vec3 & initVelocity, const uint64_t & launchTime)
{
    uint64_t currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    float timeDiff = (float)(currentTime - launchTime) / (float) MILLISECONDS_IN_SECOND;

    return (initVelocity + GRAVITY * glm::vec3(0.0f, timeDiff, 0.0f));
}

// Calculate the new position based on the current time
glm::vec3 GameEngine::calculateProjectilePosition(const glm::vec3 & initVelocity,
    const glm::vec3 & initPosition, const uint64_t & launchTime)
{
    uint64_t currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    float timeDiff = (float) (currentTime - launchTime) / (float) MILLISECONDS_IN_SECOND;

    return (initPosition + (initVelocity * (timeDiff)) + (0.5f * GRAVITY * glm::vec3(0.0f, std::pow(timeDiff, 2), 0.0f)));
}

// Calculate the new pose of the arrow that is flying
glm::mat4 GameEngine::calculateFlyingArrowPose(const rpcmsg::ArrowData & arrowData)
{
    glm::vec3 initArrowPosition = rpcmsg::rpcToGLM(arrowData.initPosition);
    glm::vec3 initArrowVelocity = rpcmsg::rpcToGLM(arrowData.initVelocity);
    uint64_t  initArrowLaunchTime = arrowData.launchTimeMilliseconds;

    glm::vec3 arrowPosition = this->calculateProjectilePosition(initArrowVelocity, initArrowPosition, initArrowLaunchTime);
    glm::vec3 nextArrowPosition = this->calculateProjectilePosition(initArrowVelocity, initArrowPosition, initArrowLaunchTime + MILLISECONDS_IN_SECOND / 200);
    glm::vec3 arrowVelocity = this->calculateProjectileVelocity(initArrowVelocity, initArrowLaunchTime);

    glm::vec3 arrowDirection = (nextArrowPosition - arrowPosition);
    float arrowYZ_Angle = ((float)glm::asin(arrowDirection.y / glm::length(arrowDirection)) + (float)M_PI) * -1.0f;
    float arrowXZ_Angle = ((float)glm::atan(arrowDirection.z / arrowDirection.x) + (float)(1.5 * M_PI)) * -1.0f;
    if (arrowDirection.x < 0.0f)
        arrowXZ_Angle += (float)M_PI;
    glm::mat4 arrowPose = glm::translate(glm::mat4(1.0f), ARROW_POSITION_OFFSET);
    arrowPose = glm::rotate(glm::mat4(1.0f), arrowYZ_Angle, glm::vec3(1.0f, 0.0f, 0.0f)) * arrowPose;
    arrowPose = glm::rotate(glm::mat4(1.0f), arrowXZ_Angle, glm::vec3(0.0f, 1.0f, 0.0f)) * arrowPose;
    arrowPose = glm::translate(glm::mat4(1.0f), arrowPosition) * arrowPose;

    return arrowPose;
}

rpcmsg::GameData GameEngine::updatePlayerData(const rpcmsg::GameData & previousGameData)
{
    // Get the new user input state
    std::unordered_map<uint32_t, rpcmsg::PlayerData> newPlayerDataInstance = this->newPlayerData;
    std::unordered_map<uint32_t, rpcmsg::PlayerData> updatedPlayerData = previousGameData.playerData;
    rpcmsg::GameData updatedGameData = previousGameData;

    // SYNCING: If new player appear, add new player
    for (auto player = newPlayerDataInstance.begin(); player != newPlayerDataInstance.end(); player++)
        if (updatedPlayerData.find(player->first) == updatedPlayerData.end())
            updatedPlayerData[player->first] = newPlayerDataInstance[player->first];

    // SYNCING: If a player disconnected, remove player
    for (auto player = updatedPlayerData.begin(); player != updatedPlayerData.end();) {
        if (newPlayerDataInstance.find(player->first) == newPlayerDataInstance.end())
            player = updatedPlayerData.erase(player);
        else
            player++;
    }

    // Note: Still old data. Not updated yet. Just has same keys as new player data
    std::unordered_map<uint32_t, rpcmsg::PlayerData> previousPlayerData = updatedPlayerData;

    // Update the state of the game based on the newly received user input
    for (auto player = newPlayerDataInstance.begin(); player != newPlayerDataInstance.end(); player++) {

        uint32_t playerID = player->first;

        // Update the user's dominant hand
        if (newPlayerDataInstance[playerID].handData[LEFT_HAND].buttonState & ovrButton::ovrButton_Y)
            updatedPlayerData[playerID].dominantHand = LEFT_HAND;
        if (newPlayerDataInstance[playerID].handData[RIGHT_HAND].buttonState & ovrButton::ovrButton_B)
            updatedPlayerData[playerID].dominantHand = RIGHT_HAND;
        
        uint32_t playerDominantHand = updatedPlayerData[playerID].dominantHand;
        uint32_t playerNonDominantHand = (playerDominantHand == LEFT_HAND) ? RIGHT_HAND : LEFT_HAND;
        glm::vec3 arrowReadyUpZone = glm::vec3((rpcmsg::rpcToGLM(newPlayerDataInstance[playerID].handData[playerNonDominantHand].handPose) * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, ARROW_READY_ZONE_Z_OFFSET)))[3]);
        glm::vec3 arrowReloadZone = glm::vec3((rpcmsg::rpcToGLM(newPlayerDataInstance[playerID].headData.headPose) * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, ARROW_RELOAD_ZONE_Z_OFFSET)))[3]);
        glm::vec3 dominantHandPosition = glm::vec3(rpcmsg::rpcToGLM(newPlayerDataInstance[playerID].handData[playerDominantHand].handPose)[3]);
        glm::vec3 nonDominantHandPosition = glm::vec3(rpcmsg::rpcToGLM(newPlayerDataInstance[playerID].handData[playerNonDominantHand].handPose)[3]);
        glm::mat4 dominantHandTransform = rpcmsg::rpcToGLM(newPlayerData[playerID].handData[playerDominantHand].handPose);

        // Player released arrow (arrowReleased == true)
        if (previousPlayerData[playerID].arrowReleased == true) {

            // See if player's arrow landed and user can pick up another arrow
            glm::vec3 arrowPosition = rpcmsg::rpcToGLM(previousPlayerData[playerID].arrowData.arrowPose)[3];
            if (arrowPosition.y < 0.0f) {
                
                // See if user is reaching for a new arrow
                if (previousPlayerData[playerID].handData[playerDominantHand].handTriggerValue < 0.5f)
                    if (newPlayerDataInstance[playerID].handData[playerDominantHand].handTriggerValue >= 0.5f)
                        if (glm::length(arrowReloadZone - dominantHandPosition) < ARROW_RELOAD_ZONE_RADIUS)
                            updatedPlayerData[playerID].arrowReleased = false;
            }
        }

        // Player is currently holding the arrow (arrowReleased == false && holding)
        else if (newPlayerDataInstance[playerID].handData[playerDominantHand].handTriggerValue > 0.5f) {
            glm::mat4 arrowPose = glm::translate(glm::mat4(1.0f), ARROW_POSITION_OFFSET);

            // Player is ready to shoot
            if (previousPlayerData[playerID].arrowReadying == true) {

                // Update arrow pose
                glm::vec3 arrowDirection = (nonDominantHandPosition - dominantHandPosition);
                float arrowYZ_Angle = ((float)glm::asin(arrowDirection.y / glm::length(arrowDirection)) + (float)M_PI) * -1.0f;
                float arrowXZ_Angle = ((float)glm::atan(arrowDirection.z / arrowDirection.x) + (float)(1.5 * M_PI)) * -1.0f;
                if (arrowDirection.x < 0.0f)
                    arrowXZ_Angle += (float)M_PI;
                glm::mat4 arrowPose = glm::translate(glm::mat4(1.0f), ARROW_POSITION_OFFSET);
                arrowPose = glm::rotate(glm::mat4(1.0f), arrowYZ_Angle, glm::vec3(1.0f, 0.0f, 0.0f)) * arrowPose;
                arrowPose = glm::rotate(glm::mat4(1.0f), arrowXZ_Angle, glm::vec3(0.0f, 1.0f, 0.0f)) * arrowPose;
                arrowPose = glm::translate(glm::mat4(1.0f), dominantHandPosition) * arrowPose;
                updatedPlayerData[playerID].arrowData.arrowPose = rpcmsg::glmToRPC(arrowPose);

                // Check if user is releasing arrow
                if (newPlayerDataInstance[playerID].handData[playerDominantHand].indexTriggerValue < 0.5f) {

                    // Store new variables for projectile calculation
                    updatedPlayerData[playerID].arrowData.initPosition = rpcmsg::glmToRPC(glm::vec3(arrowPose[3]));
                    updatedPlayerData[playerID].arrowData.initVelocity = rpcmsg::glmToRPC((nonDominantHandPosition - dominantHandPosition) * ARROW_VELOCITY_SCALE);
                    updatedPlayerData[playerID].arrowData.launchTimeMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::high_resolution_clock::now().time_since_epoch()).count();

                    updatedPlayerData[playerID].arrowReleased = true;
                    updatedPlayerData[playerID].arrowReadying = false;
                    updatedPlayerData[playerID].arrowFiringAudioCue++;

                    updatedGameData.gameState.flyingArrows.push_back(updatedPlayerData[playerID].arrowData);

                    // Comment out this line to force user to wait for arrow to land before reloading
                    updatedPlayerData[playerID].arrowData.arrowPose = rpcmsg::glmToRPC(glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f)));
                }
            }

            // Player is not readying to shoot
            else {

                // Update arrow pose
                arrowPose = dominantHandTransform * arrowPose;
                updatedPlayerData[playerID].arrowData.arrowPose = rpcmsg::glmToRPC(arrowPose);

                // Check if user is readying up to shoot
                if (glm::length(arrowReadyUpZone - dominantHandPosition) < ARROW_READY_ZONE_RADIUS) {
                    if (previousPlayerData[playerID].handData[playerDominantHand].indexTriggerValue < 0.5f) {
                        if (newPlayerDataInstance[playerID].handData[playerDominantHand].indexTriggerValue >= 0.5f) {
                            updatedPlayerData[playerID].arrowReadying = true;
                            updatedPlayerData[playerID].arrowStretchingAudioCue++;
                        }
                    }
                }
            }
        }

        // Player dropped the arrow (arrowReleased == false && !holding)
        else {
            updatedPlayerData[playerID].arrowData.arrowPose = rpcmsg::glmToRPC(glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f)));
            updatedPlayerData[playerID].arrowReleased = true;
            updatedPlayerData[playerID].arrowReadying = false;
        }

        // Update hand poses and button pressed
        updatedPlayerData[playerID].headData = newPlayerDataInstance[playerID].headData;
        updatedPlayerData[playerID].handData = newPlayerDataInstance[playerID].handData;
    }

    // Keep track of all the user's previous player data
    updatedGameData.playerData = updatedPlayerData;
    return updatedGameData;
}

rpcmsg::GameData GameEngine::updateArrowData(const rpcmsg::GameData & previousGameData) {

    // Update the arrows of each player
    rpcmsg::GameData updatedGameData = previousGameData;
    for (auto player = updatedGameData.playerData.begin(); player != updatedGameData.playerData.end(); player++) {
        
        uint32_t playerID = player->first;
        glm::vec3 arrowPosition = rpcmsg::rpcToGLM(updatedGameData.playerData[playerID].arrowData.arrowPose)[3];

        // Update arrow projectile if arrow is in the air
        if (updatedGameData.playerData[playerID].arrowReleased && (arrowPosition.y > 0.0f)) {
            glm::mat4 arrowPose = this->calculateFlyingArrowPose(updatedGameData.playerData[playerID].arrowData);
            updatedGameData.playerData[playerID].arrowData.arrowPose = rpcmsg::glmToRPC(arrowPose);
            updatedGameData.playerData[playerID].arrowData.position = rpcmsg::glmToRPC(glm::vec3(arrowPose[3]));
        }
    }

    // Update the arrows that are flying
    for (auto flyingArrow = updatedGameData.gameState.flyingArrows.begin(); flyingArrow != updatedGameData.gameState.flyingArrows.end(); ) {
        
        if (rpcmsg::rpcToGLM(flyingArrow->arrowPose)[3].y > 0.0f) {
            glm::mat4 arrowPose = this->calculateFlyingArrowPose(*flyingArrow);
            flyingArrow->arrowPose = rpcmsg::glmToRPC(arrowPose);
            flyingArrow->position = rpcmsg::glmToRPC(glm::vec3(arrowPose[3]));
            flyingArrow++;
        }
        else
            flyingArrow = updatedGameData.gameState.flyingArrows.erase(flyingArrow);
    }

    return updatedGameData;
}

rpcmsg::GameData GameEngine::updateCastleCrasher(const rpcmsg::GameData & previousGameData) {

    // Determine if arrows hit any of the castle crashers
    rpcmsg::GameData updatedGameData = previousGameData;
    for (auto player = updatedGameData.playerData.begin(); player != updatedGameData.playerData.begin(); player++) {

        uint32_t playerID = player->first;
        glm::vec3 arrowPosition = rpcmsg::rpcToGLM(updatedGameData.playerData[playerID].arrowData.arrowPose)[3];
        
        if (updatedGameData.playerData[playerID].arrowReleased && (arrowPosition.y > 0.0f)) {
            for (auto castleCrasher = updatedGameData.gameState.castleCrasherData.begin(); castleCrasher != updatedGameData.gameState.castleCrasherData.end(); castleCrasher++) {
                if (castleCrasher->alive) {
                    glm::vec3 castleCrasherPosition = rpcmsg::rpcToGLM(castleCrasher->position);
                    if (glm::length(arrowPosition - castleCrasherPosition) < CASTLE_CRASHER_HIT_RADIUS) {
                        castleCrasher->health = std::max(castleCrasher->health - (float) ARROW_DAMAGE, 0.0f);
                        castleCrasher->alive = (castleCrasher->health == 0.0f) ? false : true;
                        updatedGameData.playerData[playerID].arrowData.arrowPose = rpcmsg::glmToRPC(glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f)));
                        break;
                    }
                }
            }
        }
    }

    // Update the position of each of the castle crasher
    for (auto castleCrasher = updatedGameData.gameState.castleCrasherData.begin(); castleCrasher != updatedGameData.gameState.castleCrasherData.end(); castleCrasher++) {
        if (castleCrasher->alive) {

        }
    }

    return updatedGameData;
}

rpcmsg::GameData GameEngine::updateGameState(const rpcmsg::GameData & previousGameData)
{
    rpcmsg::GameData updatedGameData = previousGameData;

    // If game state haven't started, check to see if both users are ready
    if(previousGameData.gameState.gameStarted == false) {
        for (auto arrow = updatedGameData.gameState.flyingArrows.begin();
            arrow != updatedGameData.gameState.flyingArrows.end(); arrow++) {

            glm::vec3 arrowLocation = rpcmsg::rpcToGLM(arrow->arrowPose)[3];
            if (glm::length(arrowLocation - NOTIFICATION_SCREEN_LOCATION[0]) < READY_UP_RADIUS)
                updatedGameData.gameState.leftTowerReady = true;
            if (glm::length(arrowLocation - NOTIFICATION_SCREEN_LOCATION[1]) < READY_UP_RADIUS)
                updatedGameData.gameState.rightTowerReady = true;

            if (updatedGameData.gameState.leftTowerReady && updatedGameData.gameState.rightTowerReady) {
                updatedGameData.gameState.gameStarted = true;
                updatedGameData.gameState.gameScore = 0;
                updatedGameData.gameState.castleHealth = 100.0f;
            }
        }
    }

    // Else, check if game has ended
    else {
        if (previousGameData.gameState.castleHealth == 0.0f) {
            updatedGameData.gameState.gameStarted = false;
            updatedGameData.gameState.leftTowerReady = false;
            updatedGameData.gameState.rightTowerReady = false;
        }
    }

    return updatedGameData;
}

rpcmsg::GameData GameEngine::updateEasterEgg(const rpcmsg::GameData & previousGameData)
{
    rpcmsg::GameData updatedGameData = previousGameData;

    // Display random score if game has never started
    uint32_t currentTime = (uint32_t)std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    if (currentTime != this->easterEggLastUpdateTimer) {
        if (!previousGameData.gameState.gameStarted && (previousGameData.gameState.castleHealth != 0.0f)) {
            std::random_device randomDevice;
            std::mt19937_64 randomGenerator(randomDevice());
            std::uniform_int_distribution<unsigned long> distribution;
            uint32_t randomNumber = (uint32_t)distribution(randomGenerator);
            while (randomNumber == previousGameData.gameState.gameScore)
                randomNumber = (uint32_t)distribution(randomGenerator);
            updatedGameData.gameState.gameScore = randomNumber;
        }
    }

    this->easterEggLastUpdateTimer = currentTime;
    return updatedGameData;
}

rpcmsg::GameData GameEngine::getCopyOfGameData() {
    this->gameDataLock.lock();
    rpcmsg::GameData gameDataInstance = this->gameData;
    this->gameDataLock.unlock();
    return gameDataInstance;
}

void GameEngine::handleNewUserInput(uint32_t playerID, const rpcmsg::PlayerData & newInputs) {
    this->newPlayerData[playerID] = newInputs;
}

void GameEngine::removeUser(uint32_t playerID) {
    this->newPlayerData.erase(playerID);
}