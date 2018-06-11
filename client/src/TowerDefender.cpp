#include "TowerDefender.hpp"

#include <string>
#include <sstream>
#include <iostream>
#include <memory>
#include <vector>
#include <thread>

const bool SHOW_DEBUG_FPS = false;

TowerDefender::TowerDefender(std::string ipAddress, int portNumber) 
{
    // Create an instance of the game client to communicate with the server
    this->gameClient = std::make_unique<GameClient>(ipAddress, portNumber);
    this->sessionActive = true;
    this->playerID = 0;
    this->playerTower = 0;

    // Create new background thread to sync with server
    std::thread syncServerThread = std::thread(&TowerDefender::syncWithServer, this);
    syncServerThread.detach();

    // Create an audio session
    this->audioPlayer = std::make_unique<AudioPlayer>();
}

TowerDefender::~TowerDefender() {}

void TowerDefender::shutdownGl() {
    this->sessionActive = false;
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

void TowerDefender::initGl() {
    RiftApp::initGl();
    glClearColor(0.7f, 0.87f, 0.9f, 0.0f);
    glEnable(GL_DEPTH_TEST);
    ovr_RecenterTrackingOrigin(_session);

    // Load in shader program
    this->nonTexturedShaderID = LoadShaders(NONTEXTURED_GEOMETRY_VERTEX_SHADER_PATH, NONTEXTURED_GEOMETRY_FRAGMENT_SHADER_PATH);
    this->texturedShaderID = LoadShaders(TEXTURED_GEOMETRY_VERTEX_SHADER_PATH, TEXTURED_GEOMETRY_FRAGMENT_SHADER_PATH);

    // Load in object to draw lines
    this->lineObject = std::make_unique<Lines>();

    // Load in the environment
    this->environmentObject = std::make_unique<OBJObject>(
        std::string(ENVIRONMENT_OBJECT_PATH), MAP_SIZE);
    this->environmentTransforms = glm::mat4(1.0f);
    this->environmentTransforms = glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f)) * this->environmentTransforms;
    this->environmentTransforms = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, MAP_TRANSLATE_UP_OFFSET, 0.0f)) * this->environmentTransforms;

    // Load in castle crashers to render
    for (int i = 0; i < DIFFERENT_CASTLE_CRASHER; i++) {
        std::stringstream objectPath;
        objectPath << std::string(CASTLE_CRASHER_PATH) << std::setw(2) << std::setfill('0') << i << "/";
        this->castleCrasherObject[i].bodyObject = std::make_unique<OBJObject>(
            objectPath.str() + std::string(CASTLE_CRASHER_BODY_NAME), (float)CASTLE_CRASHER_BODY_SIZE[i]);
        this->castleCrasherObject[i].leftArmObject = std::make_unique<OBJObject>(
            objectPath.str() + std::string(CASTLE_CRASHER_LEFT_ARM_NAME), (float)CASTLE_CRASHER_LARM_SIZE[i]);
        this->castleCrasherObject[i].rightArmObject = std::make_unique<OBJObject>(
            objectPath.str() + std::string(CASTLE_CRASHER_RIGHT_ARM_NAME), (float)CASTLE_CRASHER_RARM_SIZE[i]);
        this->castleCrasherObject[i].leftLegObject = std::make_unique<OBJObject>(
            objectPath.str() + std::string(CASTLE_CRASHER_LEFT_LEG_NAME), (float)CASTLE_CRASHER_LEG_SIZE[i]);
        this->castleCrasherObject[i].rightLegObject = std::make_unique<OBJObject>(
            objectPath.str() + std::string(CASTLE_CRASHER_RIGHT_LEG_NAME), (float)CASTLE_CRASHER_LEG_SIZE[i]);
    }

    // Load in objects to render
    this->bowObject = std::make_unique<OBJObject>(std::string(BOW_OBJECT_PATH), 1.0f);
    this->arrowObject = std::make_unique<OBJObject>(std::string(ARROW_OBJECT_PATH), 1.0f);
    this->sphereObject = std::make_unique<OBJObject>(std::string(SPHERE_OBJECT_PATH), (float) HAND_SIZE);
    this->helmetObject = std::make_unique<OBJObject>(std::string(HELMET_OBJECT_PATH), (float) HELMET_SIZE);
    this->scoreTextObject = std::make_unique<OBJObject>(std::string(SCORE_TEXT_OBJECT_PATH), (float) SCORE_TEXT_SIZE);

    // Load in combo text to render
    for (int multiplier = 1; multiplier <= MAX_MULTIPLIER; multiplier *= 2)
        this->comboObject[(int)std::log2(multiplier)] = std::make_unique<OBJObject>(std::string(COMBO_MULTIPLIER_PATH) + "x" +
            std::to_string(multiplier) + ".obj", (float)MULTIPLIER_TEXT_COMBO_SIZE);

    // Load in images to render
    this->playerNotReadyImageObject = std::make_unique<Image>(std::string(PLAYER_NOT_READY_TEXTURE_PATH));

    // Load in each of the number
    for (int i = 0; i < TOTAL_SINGLE_DIGIT; i++)
        this->numberObject[i] = std::make_unique<OBJObject>(std::string(
            NUMBER_OBJECT_PATH) + std::to_string(i) + ".obj", (float)SCORE_DIGIT_SIZE);

    // Load in all the music files
    this->audioPlayer->loadAudioFile(CALM_BACKGROUND_AUDIO_PATH);
	this->audioPlayer->loadAudioFile(FESTIVE_BACKGROUND_AUDIO_PATH);
    this->audioPlayer->loadAudioFile(ARROW_FIRING_AUDIO_PATH);
    this->audioPlayer->loadAudioFile(ARROW_STRETCHING_AUDIO_PATH);
    this->audioPlayer->loadAudioFile(CONFIRMATION_AUDIO_PATH);

    // Start playing background music
    this->audioPlayer->play(CALM_BACKGROUND_AUDIO_PATH, true);

    // Get a copy of the current game state
    this->incomingGameData = this->gameClient->syncGameState();
}

void TowerDefender::update() 
{
    ovrInputState inputState;
    if (OVR_SUCCESS(ovr_GetInputState(_session, ovrControllerType_Touch, &inputState)))
    {
        // Update which tower the user is on
        if (inputState.Buttons & ovrButton::ovrButton_X)
            this->playerTower = 0;
        if (inputState.Buttons & ovrButton::ovrButton_A)
            this->playerTower = 1;
    }

    // Handle audio
    this->incomingGameDataLock.lock();
    this->currentLocalGameData = this->incomingGameData;
    this->incomingGameDataLock.unlock();
    this->handleAudioUpdate(this->currentLocalGameData, this->previousLocalGameData);

    // Update the game data
    this->previousLocalGameData = this->currentLocalGameData;

    // Store debug information
    auto currentTime = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    double fps = (double)NANOSECONDS_IN_SECOND / (double)(currentTime - lastRenderedTime);
    averageFPS += fps / 90;
    averageFpsQueue.push(fps);
    if (averageFpsQueue.size() > 90) {
        averageFPS -= averageFpsQueue.front() / 90;
        averageFpsQueue.pop();
    }
    lastRenderedTime = currentTime;
}

// Sync with server. This should be its own thread
void TowerDefender::syncWithServer()
{
    // Calculate the desired sleep duration between sync
    std::chrono::nanoseconds sleepDuration = std::chrono::nanoseconds(NANOSECONDS_IN_SECOND / SYNC_RATE);

    // Continue to update while connection is alive
    while ((this->sessionActive) && (this->gameClient->getConnectionState() == rpc::client::connection_state::connected)) 
    {
        // Request update from server and keep track of total time it took
        auto start = std::chrono::high_resolution_clock::now();
        if (this->playerID != 0) {
            bool successful = this->gameClient->updatePlayerData(this->getOculusPlayerState());
            auto updatedGameData = this->gameClient->syncGameState();
            this->incomingGameDataLock.lock();
            this->incomingGameData = updatedGameData;
            this->incomingGameDataLock.unlock();
            if (successful == false)
                this->playerID = 0;
        }

        // Sleep until the next desired sync time based on refresh rate
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::nanoseconds computeDuration =
            std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        std::this_thread::sleep_for(sleepDuration - computeDuration);
    }
}

rpcmsg::PlayerData TowerDefender::getOculusPlayerState()
{
    // Create data about the user
    rpcmsg::PlayerData playerData;
    playerData.headData.headPose = rpcmsg::glmToRPC(this->getHeadInformation());

    // Fill out data on each hand
    auto playerHandData = this->getHandInformation();
    ovrInputState inputState;
    if (OVR_SUCCESS(ovr_GetInputState(_session, ovrControllerType_Touch, &inputState)))
    {
        for (int handType = 0; handType < 2; handType++) {
            playerData.handData[handType].handPose = rpcmsg::glmToRPC(playerHandData[handType]);
            playerData.handData[handType].thumbstickValue.x = inputState.Thumbstick[handType].x;
            playerData.handData[handType].thumbstickValue.y = inputState.Thumbstick[handType].y;
            playerData.handData[handType].buttonState = inputState.Buttons;
            playerData.handData[handType].indexTriggerValue = inputState.IndexTrigger[handType];
            playerData.handData[handType].handTriggerValue = inputState.HandTrigger[handType];
        }
    }

    return playerData;
}


void TowerDefender::registerPlayer()
{
    // Initialize player data
    std::cout << "Attempting to register player..." << std::endl;
    rpcmsg::PlayerData playerData = this->getOculusPlayerState();
    playerData.arrowData.arrowPose = rpcmsg::glmToRPC(glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f)));
    playerData.dominantHand = RIGHT_HAND;
    playerData.arrowReleased = true;
    playerData.arrowReadying = false;
    playerData.arrowFiringAudioCue = 0;
    playerData.arrowStretchingAudioCue = 0;
    this->playerID = this->gameClient->registerNewPlayerSession(playerData);

    // If server does not accept new player at the moment, keep trying
    while (this->playerID == 0) {
        std::cout << "\tServer is full. Retrying..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5));
        this->playerID = this->gameClient->registerNewPlayerSession(playerData);
    }

    std::cout << "\tSuccessfully registered as playerID: " << this->playerID << std::endl;
}

void TowerDefender::handleAudioUpdate(rpcmsg::GameData & currentGameData,
     rpcmsg::GameData & previousGameData)
{
    // See if we should change background audio
    if (currentGameData.gameState.gameStarted != previousGameData.gameState.gameStarted) {
        if (currentGameData.gameState.gameStarted == true) {
            this->audioPlayer->stop(CALM_BACKGROUND_AUDIO_PATH);
            this->audioPlayer->play(FESTIVE_BACKGROUND_AUDIO_PATH, true);
        }
        else {
            this->audioPlayer->stop(FESTIVE_BACKGROUND_AUDIO_PATH);
            this->audioPlayer->play(CALM_BACKGROUND_AUDIO_PATH, true);
        }
    }

    // See if we should play audio for user shooting / stretching arrow
    if (currentGameData.playerData.find(this->playerID) != currentGameData.playerData.end()) {
        if (previousGameData.playerData.find(this->playerID) != previousGameData.playerData.end()) {
            if (currentGameData.playerData[this->playerID].arrowFiringAudioCue != previousGameData.playerData[this->playerID].arrowFiringAudioCue) {
                this->audioPlayer->play(ARROW_FIRING_AUDIO_PATH);
                this->audioPlayer->stop(ARROW_STRETCHING_AUDIO_PATH);
            }
            if (currentGameData.playerData[this->playerID].arrowStretchingAudioCue != previousGameData.playerData[this->playerID].arrowStretchingAudioCue)
                this->audioPlayer->play(ARROW_STRETCHING_AUDIO_PATH);
        }
    }

    // See if we should play confirmation audio (player hit ready)
    if (!previousGameData.gameState.leftTowerReady && currentGameData.gameState.leftTowerReady)
        this->audioPlayer->play(CONFIRMATION_AUDIO_PATH);
    if (!previousGameData.gameState.rightTowerReady && currentGameData.gameState.rightTowerReady)
        this->audioPlayer->play(CONFIRMATION_AUDIO_PATH);

    // Play sound when an enemy dies
    if (previousGameData.gameState.enemyDiedCue != currentGameData.gameState.enemyDiedCue)
        this->audioPlayer->play(CONFIRMATION_AUDIO_PATH);

}

void TowerDefender::renderScore(const glm::mat4 & projection,
    const glm::mat4 & headPose, rpcmsg::GameData & gameDataInstance)
{
    glm::mat4 scoreTextTransform = glm::translate(glm::mat4(1.0f), SCORE_TEXT_LOCATION);
    this->scoreTextObject->draw(this->nonTexturedShaderID, projection, glm::inverse(headPose), scoreTextTransform);

    uint32_t score = (SHOW_DEBUG_FPS) ? (uint32_t) this->averageFPS : gameDataInstance.gameState.gameScore;
    for (int i = 0; i < TOTAL_SCORE_DIGIT; i++) {
        glm::mat4 scoreDigitTransform = glm::translate(glm::mat4(1.0f), SCORE_CENTER_LOCATION);
        glm::vec3 positionOffset = glm::vec3((float)((TOTAL_SCORE_DIGIT / 2 - i) * SCORE_DIGIT_SPACING), 0.0f, 0.0f);
        scoreDigitTransform = glm::translate(glm::mat4(1.0f), positionOffset) * scoreDigitTransform;
        numberObject[score % 10]->draw(this->nonTexturedShaderID, projection, glm::inverse(headPose), scoreDigitTransform);
        score = score / 10;
    }
}

void TowerDefender::renderPlayers(const glm::mat4 & projection, const glm::mat4 & headPose, rpcmsg::GameData & gameDataInstance)
{
    // Draw out the user
    for (auto player = gameDataInstance.playerData.begin(); player != gameDataInstance.playerData.end(); player++) {

        uint32_t playerID = player->first;
        uint32_t playerDominantHand = gameDataInstance.playerData[playerID].dominantHand;
        uint32_t playerNonDominantHand = (playerDominantHand == LEFT_HAND) ? RIGHT_HAND : LEFT_HAND;

        glm::mat4 playerDominantHandTransform = rpcmsg::rpcToGLM(gameDataInstance.playerData[playerID].handData[playerDominantHand].handPose);
        glm::mat4 playerNonDominantHandTransform = rpcmsg::rpcToGLM(gameDataInstance.playerData[playerID].handData[playerNonDominantHand].handPose);

        // Use Oculus's hand data if rendering for current user (smoother)
        if (playerID == this->playerID) {
            std::vector<glm::mat4> systemHandInfo = this->getHandInformation();
            playerDominantHandTransform = systemHandInfo[playerDominantHand];
            playerNonDominantHandTransform = systemHandInfo[playerNonDominantHand];
        }

        glm::mat4 playerHeadPose = rpcmsg::rpcToGLM(gameDataInstance.playerData[playerID].headData.headPose);
        glm::mat4 bowTransform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.1f));
        bowTransform = playerNonDominantHandTransform * bowTransform;
        glm::mat4 arrowTransform = rpcmsg::rpcToGLM(gameDataInstance.playerData[playerID].arrowData.arrowPose);

        glm::mat4 bowStringTopTransform = bowTransform * glm::translate(glm::mat4(1.0f), ARROW_STRING_LOCATION[0]);
        glm::mat4 bowStringDownTransform = bowTransform * glm::translate(glm::mat4(1.0f), ARROW_STRING_LOCATION[1]);

        // Draw out player head
        if (playerID != this->playerID)
            this->helmetObject->draw(this->nonTexturedShaderID, projection, glm::inverse(headPose), playerHeadPose);

        // Draw out player's bow and arrow
        this->bowObject->draw(this->nonTexturedShaderID, projection, glm::inverse(headPose), bowTransform);
        this->arrowObject->draw(this->nonTexturedShaderID, projection, glm::inverse(headPose), arrowTransform);

        // Draw out player's hands
        this->sphereObject->draw(this->nonTexturedShaderID, projection, glm::inverse(headPose), playerDominantHandTransform);
        this->sphereObject->draw(this->nonTexturedShaderID, projection, glm::inverse(headPose), playerNonDominantHandTransform);

        // Draw out the string of the arrow
        if (gameDataInstance.playerData[playerID].arrowReadying) {
            this->lineObject->drawLine(bowStringTopTransform[3], playerDominantHandTransform[3],
                glm::vec3(0.0f), projection, glm::inverse(headPose));
            this->lineObject->drawLine(bowStringDownTransform[3], playerDominantHandTransform[3],
                glm::vec3(0.0f), projection, glm::inverse(headPose));
        }
        else
            this->lineObject->drawLine(bowStringTopTransform[3], bowStringDownTransform[3],
                glm::vec3(0.0f), projection, glm::inverse(headPose));

        // Draw out aim assist
        if ((playerID == this->playerID) && gameDataInstance.playerData[playerID].arrowReadying) {
            glm::vec3 initialPosition = playerDominantHandTransform[3];
            glm::vec3 initialVelocity = (playerNonDominantHandTransform[3] - playerDominantHandTransform[3]) * ARROW_VELOCITY_SCALE;
            glm::vec3 aimAssistCurrentDrawPosition = playerNonDominantHandTransform[3];
            float flyTimeInSeconds = 0.0f;
            while (aimAssistCurrentDrawPosition.y > 0.0f) {
                flyTimeInSeconds += 0.05f;
                glm::vec3 aimAssistNextDrawPosition = initialPosition + (initialVelocity *
                    (flyTimeInSeconds)+0.5f * GRAVITY * glm::vec3(0.0f, std::pow(flyTimeInSeconds, 2), 0.0f));
                this->lineObject->drawLine(aimAssistCurrentDrawPosition, aimAssistNextDrawPosition,
                    AIM_ASSIST_COLOR, projection, glm::inverse(headPose));
                aimAssistCurrentDrawPosition = aimAssistNextDrawPosition;
            }
        }
    }
}

void TowerDefender::renderFlyingArrows(const glm::mat4 & projection, const glm::mat4 & headPose, rpcmsg::GameData & gameDataInstance)
{
    // Draw out all arrows that are currently flying
    for (auto flyingArrow = gameDataInstance.gameState.flyingArrows.begin();
        flyingArrow != gameDataInstance.gameState.flyingArrows.end(); flyingArrow++) {
        glm::mat4 arrowTransform = rpcmsg::rpcToGLM(flyingArrow->arrowPose);
        this->arrowObject->draw(this->nonTexturedShaderID, projection, glm::inverse(headPose), arrowTransform);
    }
}

void TowerDefender::renderCastleCrashers(const glm::mat4 & projection, const glm::mat4 & headPose, rpcmsg::GameData & gameDataInstance)
{
    for (auto castleCrasher = gameDataInstance.gameState.castleCrasherData.begin();
        castleCrasher != gameDataInstance.gameState.castleCrasherData.end(); castleCrasher++) {

        // Calculate the rotation of the legs/arms
        float rotationAngle = (castleCrasher->animationCycle <= 180.0f) ? castleCrasher->animationCycle : (360.0f - castleCrasher->animationCycle);
        float leftArmRotation = rotationAngle / 1.5f - 60.0f;
        float rightArmRotation = rotationAngle / -1.5f - 60.0f;
        float leftLegRotation = rotationAngle / 1.5f - 60.0f;
        float rightLegRotation = rotationAngle / -1.5f + 60.0f;

        // Calculate transforms of castle crasher and draw them out
        int castleCrasherID = castleCrasher->id % DIFFERENT_CASTLE_CRASHER;
        glm::mat4 leftArmTransform = glm::translate(glm::mat4(1.0f), CASTLE_CRASHER_LEFT_ARM_PRE_ROTATION[castleCrasherID]);
        leftArmTransform = glm::rotate(glm::mat4(1.0f), glm::radians(leftArmRotation), glm::vec3(1.0f, 0.0f, 0.0f)) * leftArmTransform;
        leftArmTransform = glm::translate(glm::mat4(1.0f), CASTLE_CRASHER_LEFT_ARM_OFFSET[castleCrasherID]) * leftArmTransform;
        glm::mat4 rightArmTransform = glm::translate(glm::mat4(1.0f), CASTLE_CRASHER_RIGHT_ARM_PRE_ROTATION[castleCrasherID]);
        rightArmTransform = glm::rotate(glm::mat4(1.0f), glm::radians(rightArmRotation), glm::vec3(1.0f, 0.0f, 0.0f)) * rightArmTransform;
        rightArmTransform = glm::translate(glm::mat4(1.0f), CASTLE_CRASHER_RIGHT_ARM_OFFSET[castleCrasherID]) * rightArmTransform;
        glm::mat4 leftLegTransform = glm::translate(glm::mat4(1.0f), CASTLE_CRASHER_LEFT_LEG_PRE_ROTATION[castleCrasherID]);
        leftLegTransform = glm::rotate(glm::mat4(1.0f), glm::radians(leftLegRotation), glm::vec3(1.0f, 0.0f, 0.0f)) * leftLegTransform;
        leftLegTransform = glm::translate(glm::mat4(1.0f), CASTLE_CRASHER_LEFT_LEG_OFFSET[castleCrasherID]) * leftLegTransform;
        glm::mat4 rightLegTransform = glm::translate(glm::mat4(1.0f), CASTLE_CRASHER_RIGHT_LEG_PRE_ROTATION[castleCrasherID]);
        rightLegTransform = glm::rotate(glm::mat4(1.0f), glm::radians(rightLegRotation), glm::vec3(1.0f, 0.0f, 0.0f)) * rightLegTransform;
        rightLegTransform = glm::translate(glm::mat4(1.0f), CASTLE_CRASHER_RIGHT_LEG_OFFSET[castleCrasherID]) * rightLegTransform;

        float arrowXZ_Angle = ((float)glm::atan(castleCrasher->direction.z / castleCrasher->direction.x) + (float)(1.5 * M_PI)) * -1.0f;
        arrowXZ_Angle += (castleCrasher->direction.x < 0.0f) ? (float) M_PI : 0.0f;
        glm::mat4 castleCrasherRotation = glm::rotate(glm::mat4(1.0f), arrowXZ_Angle, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 castleCrasherLocation = glm::translate(glm::mat4(1.0f), rpcmsg::rpcToGLM(castleCrasher->position));
        glm::mat4 castleCrasherTransform = castleCrasherLocation * castleCrasherRotation;

        // Draw out the castle crasher
        this->castleCrasherObject[castleCrasherID].bodyObject->draw(this->nonTexturedShaderID, projection,
            glm::inverse(headPose), castleCrasherTransform);
        this->castleCrasherObject[castleCrasherID].leftArmObject->draw(this->nonTexturedShaderID, projection,
            glm::inverse(headPose), castleCrasherTransform * leftArmTransform);
        this->castleCrasherObject[castleCrasherID].rightArmObject->draw(this->nonTexturedShaderID, projection,
            glm::inverse(headPose), castleCrasherTransform * rightArmTransform);
        this->castleCrasherObject[castleCrasherID].leftLegObject->draw(this->nonTexturedShaderID, projection,
            glm::inverse(headPose), castleCrasherTransform * leftLegTransform);
        this->castleCrasherObject[castleCrasherID].rightLegObject->draw(this->nonTexturedShaderID, projection,
            glm::inverse(headPose), castleCrasherTransform * rightLegTransform);
    }

    keke += 1.0f;
    if (keke > 360.0f)
        keke = 0.0f;

}
void TowerDefender::renderCastleHealth(const glm::mat4 & projection, const glm::mat4 & headPose, rpcmsg::GameData & gameDataInstance)
{
    // Draw out the castle health bar
    glm::vec3 healthBarRemainingLifeLocation = CASTLE_HEALTH_BAR_START_LOCATION;
    healthBarRemainingLifeLocation.x -= gameDataInstance.gameState.castleHealth / 100.0f * HEALTH_BAR_WIDTH;
    glm::vec3 healthBarEndLocation = CASTLE_HEALTH_BAR_START_LOCATION;
    healthBarEndLocation.x -= HEALTH_BAR_WIDTH;
    this->lineObject->drawLine(CASTLE_HEALTH_BAR_START_LOCATION, healthBarRemainingLifeLocation,
        glm::vec3(0.0f, 1.0f, 0.0f), projection, glm::inverse(headPose), (float)HEALTH_BAR_SIZE);
    this->lineObject->drawLine(healthBarRemainingLifeLocation, healthBarEndLocation,
        glm::vec3(1.0f, 0.0f, 0.0f), projection, glm::inverse(headPose), (float)HEALTH_BAR_SIZE);

}

void TowerDefender::renderNotification(const glm::mat4 & projection, const glm::mat4 & headPose, rpcmsg::GameData & gameDataInstance)
{
    // Draw out the notification menu
    for (int i = 0; i < NOTIFICATION_SCREEN_LOCATION.size(); i++) {
        if (((i == 0) ? gameDataInstance.gameState.leftTowerReady : gameDataInstance.gameState.rightTowerReady) == false) {
            glm::mat4 screenTransform = glm::scale(glm::mat4(1.0f), glm::vec3((float)NOTIFICATION_SIZE));
            screenTransform = glm::rotate(glm::mat4(1.0f), glm::radians((i == 0) ? 45.0f : -45.0f), glm::vec3(0.0f, 1.0f, 0.0f)) * screenTransform;
            screenTransform = glm::translate(glm::mat4(1.0f), NOTIFICATION_SCREEN_LOCATION[i]) * screenTransform;
            this->playerNotReadyImageObject->draw(this->texturedShaderID, projection, glm::inverse(headPose), screenTransform);
        }
    }
}

void TowerDefender::renderComboText(const glm::mat4 & projection, const glm::mat4 & headPose, rpcmsg::GameData & gameDataInstance)
{
    // Draw out the combo text so it faces the user
    for (auto multiplierText = gameDataInstance.gameState.multiplierDisplayData.begin();
        multiplierText != gameDataInstance.gameState.multiplierDisplayData.end(); multiplierText++) {
        
        glm::vec3 playerHeadLocation = rpcmsg::rpcToGLM(gameDataInstance.playerData[playerID].headData.headPose)[3];
        glm::vec3 multiplierLocation = rpcmsg::rpcToGLM(multiplierText->pose)[3];
        glm::vec3 direction = playerHeadLocation - multiplierLocation;
        float arrowXZ_Angle = ((float)glm::atan(direction.z / direction.x) + (float)(1.5 * M_PI)) * -1.0f;
        arrowXZ_Angle += (direction.x < 0.0f) ? (float)M_PI : 0.0f;
        
        glm::mat4 comboTextTransform = glm::rotate(glm::mat4(1.0f), arrowXZ_Angle, glm::vec3(0.0f, 1.0f, 0.0f));
        comboTextTransform = rpcmsg::rpcToGLM(multiplierText->pose) * comboTextTransform;
        this->comboObject[(int)std::log2(multiplierText->multiplier)]->draw(this->nonTexturedShaderID,
            projection, glm::inverse(headPose), comboTextTransform, multiplierText->opacity);
    }
}

void TowerDefender::renderScene(const glm::mat4 & projection, const glm::mat4 & headPose)
{
    // Register player if we haven't. This is a blocking call
    if (this->playerID == 0)
        this->registerPlayer();

    // Calculate user translated position
    glm::mat4 translatedHeadPose = glm::translate(glm::mat4(1.0f), USER_TRANSLATION[playerTower]) * headPose;

    // Draw out the environment
    this->environmentObject->draw(this->nonTexturedShaderID, projection, glm::inverse(translatedHeadPose), this->environmentTransforms);

    // Get a copy of the current data
    rpcmsg::GameData gameDataInstance = this->currentLocalGameData;

    // Perform render procedure
    this->renderScore(projection, translatedHeadPose, gameDataInstance);
    this->renderPlayers(projection, translatedHeadPose, gameDataInstance);
    this->renderFlyingArrows(projection, translatedHeadPose, gameDataInstance);
    this->renderCastleCrashers(projection, translatedHeadPose, gameDataInstance);
    this->renderCastleHealth(projection, translatedHeadPose, gameDataInstance);
    this->renderNotification(projection, translatedHeadPose, gameDataInstance);
    this->renderComboText(projection, translatedHeadPose, gameDataInstance);
}

glm::mat4 TowerDefender::getHeadInformation() 
{
    // Query Touch controllers. Query their parameters:
    double displayMidpointSeconds = ovr_GetPredictedDisplayTime(_session, 0);
    ovrTrackingState trackState = ovr_GetTrackingState(_session, displayMidpointSeconds, ovrTrue);
    return (glm::translate(glm::mat4(1.0f), USER_TRANSLATION[this->playerTower]) * ovr::toGlm(trackState.HeadPose.ThePose));
}

std::vector<glm::mat4> TowerDefender::getHandInformation()
{
    // Query Touch controllers. Query their parameters:
    double displayMidpointSeconds = ovr_GetPredictedDisplayTime(_session, 0);
    ovrTrackingState trackState = ovr_GetTrackingState(_session, displayMidpointSeconds, ovrTrue);

    // Process controller status. Useful to know if controller is being used at all, and if the cameras can see it. 
    // Bits reported:
    // Bit 1: ovrStatus_OrientationTracked  = Orientation is currently tracked (connected and in use)
    // Bit 2: ovrStatus_PositionTracked     = Position is currently tracked (false if out of range)
    unsigned int handStatus[2];
    handStatus[0] = trackState.HandStatusFlags[0];
    handStatus[1] = trackState.HandStatusFlags[1];

    // Process controller position and orientation:
    ovrPosef handPoses[2];  // These are position and orientation in meters in room coordinates, relative to tracking origin. Right-handed cartesian coordinates.
                            // ovrQuatf     Orientation;
                            // ovrVector3f  Position;
    handPoses[ovrHand_Left] = trackState.HandPoses[ovrHand_Left].ThePose;
    handPoses[ovrHand_Right] = trackState.HandPoses[ovrHand_Right].ThePose;

    std::vector<glm::mat4> handInformation;
    handInformation.push_back(glm::translate(glm::mat4(1.0f), USER_TRANSLATION[this->playerTower]) * ovr::toGlm(handPoses[0]));
    handInformation.push_back(glm::translate(glm::mat4(1.0f), USER_TRANSLATION[this->playerTower]) * ovr::toGlm(handPoses[1]));
    return handInformation;
}

// Start the Tower Defender Oculus game
//int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
int main(int argc, char** argv)
{
    std::string ipAddress;
    int portNumber;

    std::cout << "Please enter IP address  of server: ";
    std::cin >> ipAddress;

    std::cout << "Please enter port number of server: ";
    std::cin >> portNumber;

    // Launch the game on the Oculus Rift
    int result = -1;
    try {
        if (!OVR_SUCCESS(ovr_Initialize(nullptr))) {
            FAIL("Failed to initialize the Oculus SDK");
        }
        result = TowerDefender(ipAddress, portNumber).run();
    }
    catch (std::exception & error) {
        OutputDebugStringA(error.what());
        std::cerr << error.what() << std::endl;
    }
    ovr_Shutdown();

    return 0;
}