#include "TowerDefender.hpp"

#include <string>
#include <iostream>
#include <memory>
#include <vector>
#include <thread>

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

    // Load in objects to render
    this->bowObject = std::make_unique<OBJObject>(std::string(BOW_OBJECT_PATH), 1.0f);
    this->arrowObject = std::make_unique<OBJObject>(std::string(ARROW_OBJECT_PATH), 1.0f);
    this->sphereObject = std::make_unique<OBJObject>(std::string(SPHERE_OBJECT_PATH), (float) HAND_SIZE);
    this->helmetObject = std::make_unique<OBJObject>(std::string(HELMET_OBJECT_PATH), (float) HELMET_SIZE);
    this->scoreTextObject = std::make_unique<OBJObject>(std::string(SCORE_TEXT_OBJECT_PATH), (float) SCORE_TEXT_SIZE);

    // Load in images to render
    this->playerNotReadyImageObject = std::make_unique<Image>(std::string(PLAYER_NOT_READY_TEXTURE_PATH));

    // Load in each of the number
    for (int i = 0; i < TOTAL_SINGLE_DIGIT; i++)
        this->numberObject[i] = std::make_unique<OBJObject>(std::string(
            NUMBER_OBJECT_PATH) + std::to_string(i) + ".obj", (float)SCORE_DIGIT_SIZE);

    // Load in all the music files
    this->audioPlayer->loadAudioFile(CALM_BACKGROUND_AUDIO_PATH);
    this->audioPlayer->loadAudioFile(ARROW_FIRING_AUDIO_PATH);
    this->audioPlayer->loadAudioFile(ARROW_STRETCHING_AUDIO_PATH);

    // Start playing background music
    this->audioPlayer->play(CALM_BACKGROUND_AUDIO_PATH, true);

    // Get a copy of the current game state
    this->gameData = this->gameClient->syncGameState();
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
            this->gameDataLock.lock();
            this->gameData = updatedGameData;
            this->gameDataLock.unlock();
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
    /**
    if (currentGameData.gameState.gameStarted != previousGameData.gameState.gameStarted) {
        if (currentGameData.gameState.gameStarted == true)
            this->audioPlayer->play(CALM_BACKGROUND_AUDIO_PATH, true);
        else
            this->audioPlayer->play(CALM_BACKGROUND_AUDIO_PATH, true);
    }*/

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

}

void TowerDefender::renderScore(const glm::mat4 & projection,
    const glm::mat4 & headPose, rpcmsg::GameData & currentGameData)
{
    glm::mat4 scoreTextTransform = glm::translate(glm::mat4(1.0f), SCORE_TEXT_LOCATION);
    this->scoreTextObject->draw(this->nonTexturedShaderID, projection, glm::inverse(headPose), scoreTextTransform);

    uint32_t score = currentGameData.gameState.gameScore;
    for (int i = 0; i < TOTAL_SCORE_DIGIT; i++) {
        glm::mat4 scoreDigitTransform = glm::translate(glm::mat4(1.0f), SCORE_CENTER_LOCATION);
        glm::vec3 positionOffset = glm::vec3((float)((TOTAL_SCORE_DIGIT / 2 - i) * SCORE_DIGIT_SPACING), 0.0f, 0.0f);
        scoreDigitTransform = glm::translate(glm::mat4(1.0f), positionOffset) * scoreDigitTransform;
        numberObject[score % 10]->draw(this->nonTexturedShaderID, projection, glm::inverse(headPose), scoreDigitTransform);
        score = score / 10;
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

    // Draw out the user
    this->gameDataLock.lock();
    rpcmsg::GameData gameDataInstance = this->gameData;
    this->gameDataLock.unlock();
    for (auto player = gameDataInstance.playerData.begin(); player != gameDataInstance.playerData.end(); player++) {
        
        uint32_t playerID = player->first;
        uint32_t playerDominantHand = gameDataInstance.playerData[playerID].dominantHand;
        uint32_t playerNonDominantHand = (playerDominantHand == LEFT_HAND) ? RIGHT_HAND : LEFT_HAND;

        glm::mat4 playerDominantHandTransform = rpcmsg::rpcToGLM(gameDataInstance.playerData[playerID].handData[playerDominantHand].handPose);
        glm::mat4 playerNonDominantHandTransform = rpcmsg::rpcToGLM(gameDataInstance.playerData[playerID].handData[playerNonDominantHand].handPose);

        // Use Oculus's hand data if rendering for current user (smoother)
        /**
        if (playerID == this->playerID) {
            std::vector<glm::mat4> systemHandInfo = this->getHandInformation();
            playerDominantHandTransform = systemHandInfo[playerDominantHand];
            playerNonDominantHandTransform = systemHandInfo[playerNonDominantHand];
        }*/
        
        glm::mat4 playerHeadPose = rpcmsg::rpcToGLM(gameDataInstance.playerData[playerID].headData.headPose);
        glm::mat4 bowTransform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.1f));
        bowTransform = playerNonDominantHandTransform * bowTransform;
        glm::mat4 arrowTransform = rpcmsg::rpcToGLM(gameDataInstance.playerData[playerID].arrowData.arrowPose);

        glm::mat4 bowStringTopTransform = bowTransform * glm::translate(glm::mat4(1.0f), ARROW_STRING_LOCATION[0]);
        glm::mat4 bowStringDownTransform = bowTransform * glm::translate(glm::mat4(1.0f), ARROW_STRING_LOCATION[1]);

        // Draw out player head
        if (playerID != this->playerID)
            this->helmetObject->draw(this->nonTexturedShaderID, projection, glm::inverse(translatedHeadPose), playerHeadPose);

        // Draw out player's bow and arrow
        this->bowObject->draw(this->nonTexturedShaderID, projection, glm::inverse(translatedHeadPose), bowTransform);
        this->arrowObject->draw(this->nonTexturedShaderID, projection, glm::inverse(translatedHeadPose), arrowTransform);
        this->sphereObject->draw(this->nonTexturedShaderID, projection, glm::inverse(translatedHeadPose), arrowTransform);

        // Draw out player's hands
        this->sphereObject->draw(this->nonTexturedShaderID, projection, glm::inverse(translatedHeadPose), playerDominantHandTransform);
        this->sphereObject->draw(this->nonTexturedShaderID, projection, glm::inverse(translatedHeadPose), playerNonDominantHandTransform);

        // Draw out the string of the arrow
        if (gameDataInstance.playerData[playerID].arrowReadying) {
            this->lineObject->drawLine(bowStringTopTransform[3], playerDominantHandTransform[3],
                glm::vec3(0.0f), projection, glm::inverse(translatedHeadPose));
            this->lineObject->drawLine(bowStringDownTransform[3], playerDominantHandTransform[3],
                glm::vec3(0.0f), projection, glm::inverse(translatedHeadPose));
        }
        else
            this->lineObject->drawLine(bowStringTopTransform[3], bowStringDownTransform[3],
                glm::vec3(0.0f), projection, glm::inverse(translatedHeadPose));

        // Draw out aim assist
        if ((playerID == this->playerID) && gameDataInstance.playerData[playerID].arrowReadying) {
            glm::vec3 initialPosition = playerDominantHandTransform[3];
            glm::vec3 initialVelocity = (playerNonDominantHandTransform[3] - playerDominantHandTransform[3]) * ARROW_VELOCITY_SCALE;
            glm::vec3 aimAssistCurrentDrawPosition = playerNonDominantHandTransform[3];
            float flyTimeInSeconds = 0.0f;
            while (aimAssistCurrentDrawPosition.y > 0.0f) {
                flyTimeInSeconds += 0.05f;
                glm::vec3 aimAssistNextDrawPosition = initialPosition + (initialVelocity * 
                    (flyTimeInSeconds)+ 0.5f * GRAVITY * glm::vec3(0.0f, std::pow(flyTimeInSeconds, 2), 0.0f));
                this->lineObject->drawLine(aimAssistCurrentDrawPosition, aimAssistNextDrawPosition, 
                    AIM_ASSIST_COLOR, projection, glm::inverse(translatedHeadPose));
                aimAssistCurrentDrawPosition = aimAssistNextDrawPosition;
            }
        }
    }

    // Draw out all arrows that are currently flying
    for (auto flyingArrow = gameDataInstance.gameState.flyingArrows.begin(); 
        flyingArrow != gameDataInstance.gameState.flyingArrows.end(); flyingArrow++) {
        glm::mat4 arrowTransform = rpcmsg::rpcToGLM(flyingArrow->arrowPose);
        this->arrowObject->draw(this->nonTexturedShaderID, projection, glm::inverse(translatedHeadPose), arrowTransform);
        this->sphereObject->draw(this->nonTexturedShaderID, projection, glm::inverse(translatedHeadPose), arrowTransform);
    }

    // Draw out the notification menu
    for (int i = 0; i < NOTIFICATION_SCREEN_LOCATION.size(); i++) {
        if (((i == 0) ? gameDataInstance.gameState.leftTowerReady : gameDataInstance.gameState.rightTowerReady) == false) {
            glm::mat4 screenTransform = glm::scale(glm::mat4(1.0f), glm::vec3((float)NOTIFICATION_SIZE));
            screenTransform = glm::rotate(glm::mat4(1.0f), glm::radians((i == 0) ? 45.0f : -45.0f), glm::vec3(0.0f, 1.0f, 0.0f)) * screenTransform;
            screenTransform = glm::translate(glm::mat4(1.0f), NOTIFICATION_SCREEN_LOCATION[i]) * screenTransform;
            this->playerNotReadyImageObject->draw(this->texturedShaderID, projection, glm::inverse(translatedHeadPose), screenTransform);
        }
    }

    // Draw out the score
    this->renderScore(projection, translatedHeadPose, gameDataInstance);

    // Handle audio
    this->handleAudioUpdate(gameDataInstance, this->previousGameData);

    // Update the game data
    this->previousGameData = gameDataInstance;
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