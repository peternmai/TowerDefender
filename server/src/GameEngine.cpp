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
    updatedGameData = this->updateMultiplierDisplay(updatedGameData);
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
    glm::vec3 nextArrowPosition = this->calculateProjectilePosition(initArrowVelocity, initArrowPosition, initArrowLaunchTime - MILLISECONDS_IN_SECOND / 200);
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
    this->newPlayerDataLock.lock();
    std::unordered_map<uint32_t, rpcmsg::PlayerData> newPlayerDataInstance = this->newPlayerData;
    this->newPlayerDataLock.unlock();
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
                    updatedPlayerData[playerID].arrowData.arrowPose = rpcmsg::glmToRPC(glm::translate(glm::mat4(1.0f), glm::vec3(-5.0f)));
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
            updatedPlayerData[playerID].arrowData.arrowPose = rpcmsg::glmToRPC(glm::translate(glm::mat4(1.0f), glm::vec3(-5.0f)));
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

    rpcmsg::GameData updatedGameData = previousGameData;

    // Calculate time for time based calculation
    std::chrono::nanoseconds currentTime = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch());
    std::chrono::nanoseconds startTime = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::seconds(MAX_DIFFICULTY_SECONDS));

    // Determine if arrows hit any of the castle crashers
    for (auto arrow = updatedGameData.gameState.flyingArrows.begin(); arrow != updatedGameData.gameState.flyingArrows.end();) {
        auto nextArrow = std::next(arrow);

        // Check each castle crasher with this arrow
        glm::vec3 arrowPosition = rpcmsg::rpcToGLM(arrow->arrowPose)[3];
        for (auto castleCrasher = updatedGameData.gameState.castleCrasherData.begin(); castleCrasher != updatedGameData.gameState.castleCrasherData.end(); castleCrasher++) {
            if (castleCrasher->alive) {
                glm::vec3 castleCrasherPosition = rpcmsg::rpcToGLM(castleCrasher->position);

                // See if this arrow hit castle crasher
                if (glm::length(arrowPosition - castleCrasherPosition) < CASTLE_CRASHER_HIT_RADIUS) {
                    castleCrasher->health = std::max(castleCrasher->health - ARROW_DAMAGE, 0.0f);
                    castleCrasher->alive = (castleCrasher->health == 0.0f) ? false : true;

                    // If castle crasher died, update score
                    if (castleCrasher->alive == false) {
                        updatedGameData.gameState.castleCrasherData.erase(castleCrasher);
                        if ((currentTime - this->lastHitTime).count() < (COMBO_TIME_SECONDS * NANOSECONDS_IN_SECOND))
                            this->comboMultiplier = std::min(this->comboMultiplier * 2.0f, (float)MAX_MULTIPLIER);
                        else
                            this->comboMultiplier = 1.0f;
                        updatedGameData.gameState.gameScore += (uint32_t)(this->comboMultiplier * BASE_POINTS_PER_HIT);
                        updatedGameData.gameState.enemyDiedCue++;

                        // Add multiplier
                        rpcmsg::MultiplierDisplayData newMultiplierDisplayData;
                        glm::vec3 multiplierLocation = castleCrasherPosition;
                        multiplierLocation.y += 5.0f;
                        newMultiplierDisplayData.pose = rpcmsg::glmToRPC(glm::translate(glm::mat4(1.0f), multiplierLocation));
                        newMultiplierDisplayData.opacity = 1.0f;
                        newMultiplierDisplayData.multiplier = (uint32_t) this->comboMultiplier;
                        updatedGameData.gameState.multiplierDisplayData.push_back(newMultiplierDisplayData);
                    }
                    this->lastHitTime = currentTime;
                    updatedGameData.gameState.flyingArrows.erase(arrow);
                    break;
                }
            }
        }
        arrow = nextArrow;
    }

    // Create a random generator for castle crasher generation / movement update
    std::random_device randomDevice;
    std::mt19937_64 randomGenerator(randomDevice());
    std::uniform_int_distribution<unsigned long> distribution;

    // See if we should spawn new castle crashers
    if (updatedGameData.gameState.gameStarted == true) {

        // Figure out the ideal number of castle crashers to show at this time
        double idealCastleCrasherPercentAlive = ((currentTime - startTime).count()) / (double)(NANOSECONDS_IN_SECOND * MAX_DIFFICULTY_SECONDS);
        idealCastleCrasherPercentAlive = std::min(idealCastleCrasherPercentAlive, 1.0);
        int idealCastleCrasherAlive = (int)(MAX_CASTLE_CRASHERS * idealCastleCrasherPercentAlive);

        // If ideal is higher than actual, see if we should spawn a new castle crasher
        if (idealCastleCrasherAlive > updatedGameData.gameState.castleCrasherData.size()) {
            if (currentTime > (lastSpawnTime + spawnCooldownTimer)) {

                // Initialize new castle crasher and add them
                rpcmsg::CastleCrasherData newCastleCrasher;
                newCastleCrasher.id = (uint8_t)distribution(randomGenerator);
                newCastleCrasher.alive = true;
                newCastleCrasher.health = 100.0f;
                newCastleCrasher.animationCycle = (float)(distribution(randomGenerator) % 360);
                newCastleCrasher.direction = rpcmsg::glmToRPC(glm::vec3(0.0f, 0.0f, 1.0f));
                float spawnPositionX = (float)(distribution(randomGenerator) %
                    (long long)(CASTLE_CRASHER_MAX_X - CASTLE_CRASHER_MIN_X)) + CASTLE_CRASHER_MIN_X;
                float spawnPositionZ = (float)(distribution(randomGenerator) %
                    (long long)(SPAWN_Z_RANGE)) + CASTLE_CRASHER_MIN_Z;
                newCastleCrasher.position = rpcmsg::glmToRPC(glm::vec3(spawnPositionX, -3.0f, spawnPositionZ));
                float endPositionX = (float)(distribution(randomGenerator) % (long long)(CHEST_MAX_X - CHEST_MIN_X)) + CHEST_MIN_X;
                newCastleCrasher.endPosition = rpcmsg::glmToRPC(glm::vec3(endPositionX, 0.5f, CHEST_Z));
                newCastleCrasher.lastAttackTimeMilliseconds = 0;
                updatedGameData.gameState.castleCrasherData.push_back(newCastleCrasher);

                // Update spawn cooldown timer
                float spawnTimeRandom = (float)(distribution(randomGenerator) % 1000) / 1000.0f;
                float spawnCooldownSeconds = (60.0f / (MAX_CASTLE_CRASHERS / (MAX_DIFFICULTY_SECONDS / 60.0f))) * 2.0f * spawnTimeRandom;
                this->spawnCooldownTimer = std::chrono::nanoseconds((long long)(spawnCooldownSeconds * NANOSECONDS_IN_SECOND));
                this->lastSpawnTime = currentTime;
            }
        }
    }

    // Update the position of each of the castle crasher and attack damge to chest
    for (auto castleCrasher = updatedGameData.gameState.castleCrasherData.begin(); castleCrasher != updatedGameData.gameState.castleCrasherData.end(); castleCrasher++) {
        if (castleCrasher->alive) {
            castleCrasher->animationCycle += ((1.0f / ANIMATION_TIME_SECONDS) * (1.0f / REFRESH_RATE)) * 360.0f;
            if (castleCrasher->animationCycle >= 360.0f)
                castleCrasher->animationCycle = 0.0f;

            // Castle crasher is walking to chest
            if (castleCrasher->position.z < CHEST_Z) {

                // Update castle crasher direction
                glm::vec3 direction = rpcmsg::rpcToGLM(castleCrasher->endPosition) - rpcmsg::rpcToGLM(castleCrasher->position);
                castleCrasher->direction = rpcmsg::glmToRPC(direction);

                // Calculate the castle crasher delta position in one second
                glm::vec3 deltaPosition = glm::normalize(direction) * CASTLE_CRASHER_WALK_SPEED;
                    
                // Calculate the castle crasher updated position
                glm::vec3 newPosition = rpcmsg::rpcToGLM(castleCrasher->position);
                newPosition += deltaPosition / (float)REFRESH_RATE;

                // Account for hills
                float desiredY = 0.5f;
                if (glm::length(glm::vec2(-40.0f, -40.0f) - glm::vec2(newPosition.x, newPosition.z)) < 30.0f)
                    desiredY += ((30.0f - glm::length(glm::vec2(-40.0f, -40.0f) - glm::vec2(newPosition.x, newPosition.z))) / 30.0f) * 5.0f;
                if (glm::length(glm::vec2(26.0f, -80.0f) - glm::vec2(newPosition.x, newPosition.z)) < 20.0f)
                    desiredY += ((20.0f - glm::length(glm::vec2(26.0f, -80.0f) - glm::vec2(newPosition.x, newPosition.z))) / 20.0f) * 7.0f;
                if (glm::length(glm::vec2(78.0f, -32.0f) - glm::vec2(newPosition.x, newPosition.z)) < 50.0f)
                    desiredY += ((50.0f - glm::length(glm::vec2(78.0f, -32.0f) - glm::vec2(newPosition.x, newPosition.z))) / 50.0f) * 10.0f;

                // If enemy recently spawn, gradually move enemy to surface
                if (std::abs(desiredY - newPosition.y) > 0.1f)
                    newPosition.y += (((desiredY - newPosition.y) > 0) ? 3.0f : -3.0f) / REFRESH_RATE;

                castleCrasher->position = rpcmsg::glmToRPC(newPosition);

            }

            // Castle crasher is attacking chest
            else {
                long long nextAttackReadyAt = (castleCrasher->lastAttackTimeMilliseconds * MILLI_TO_NANOSECONDS) +
                    (long long)((double)CASTLE_CRASHER_ATTACK_SPEED * NANOSECONDS_IN_SECOND);
                if (nextAttackReadyAt < currentTime.count()) {
                    updatedGameData.gameState.castleHealth = std::max(0.0f,
                        updatedGameData.gameState.castleHealth - CASTLE_CRASHER_DAMAGE);
                    castleCrasher->lastAttackTimeMilliseconds = (uint32_t)(currentTime.count() / MILLI_TO_NANOSECONDS);
                }
                
            }
        }
    }

    return updatedGameData;
}

rpcmsg::GameData GameEngine::updateMultiplierDisplay(const rpcmsg::GameData & previousGameData)
{
    rpcmsg::GameData updatedGameData = previousGameData;
    for (auto multiplierData = updatedGameData.gameState.multiplierDisplayData.begin();
        multiplierData != updatedGameData.gameState.multiplierDisplayData.end();) {
        
        // No more opacity, don't have to render anymore
        if (multiplierData->opacity <= 0.0f)
            multiplierData = updatedGameData.gameState.multiplierDisplayData.erase(multiplierData);

        // Else, update how to draw
        else {
            glm::vec3 multiplierLocation = rpcmsg::rpcToGLM(multiplierData->pose)[3];
            multiplierLocation.y += 1.0f / REFRESH_RATE;
            multiplierData->pose = rpcmsg::glmToRPC(glm::translate(glm::mat4(1.0f), multiplierLocation));
            multiplierData->opacity -= 1.0f / REFRESH_RATE;
            multiplierData++;
        }
    }

    return updatedGameData;
}

rpcmsg::GameData GameEngine::updateGameState(const rpcmsg::GameData & previousGameData)
{
    rpcmsg::GameData updatedGameData = previousGameData;

    // Update multiplier
    std::chrono::nanoseconds currentTime = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch());
    if ((currentTime - this->lastHitTime).count() > (COMBO_TIME_SECONDS * NANOSECONDS_IN_SECOND))
        this->comboMultiplier = 1.0f;
    updatedGameData.gameState.scoreMultiplier = (uint32_t) this->comboMultiplier;

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
                updatedGameData.gameState.enemyDiedCue = 0;
            }
        }
    }

    // Else, check if game has ended
    else {
        if (previousGameData.gameState.castleHealth == 0.0f) {
            updatedGameData.gameState.gameStarted = false;
            updatedGameData.gameState.leftTowerReady = false;
            updatedGameData.gameState.rightTowerReady = false;
            std::list<rpcmsg::CastleCrasherData>().swap(updatedGameData.gameState.castleCrasherData);
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
    this->newPlayerDataLock.lock();
    this->newPlayerData[playerID] = newInputs;
    this->newPlayerDataLock.unlock();
}

void GameEngine::removeUser(uint32_t playerID) {
    this->newPlayerDataLock.lock();
    this->newPlayerData.erase(playerID);
    this->newPlayerDataLock.unlock();
}