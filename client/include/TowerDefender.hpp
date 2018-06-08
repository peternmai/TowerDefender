#pragma once

#include <chrono>
#include <mutex>
#include <array>

#include "MinimalOculus.hpp"
#include "GameClient.hpp"
#include "AudioPlayer.hpp"
#include "OBJObject.hpp"
#include "Lines.hpp"
#include "Image.hpp"
#include "shader.hpp"
#include "rpcMessages.hpp"

#define ENVIRONMENT_OBJECT_PATH "../objects/environment.obj"
#define BOW_OBJECT_PATH         "../objects/bow.obj"
#define ARROW_OBJECT_PATH       "../objects/arrow.obj"
#define SPHERE_OBJECT_PATH      "../objects/sphere.obj"
#define HELMET_OBJECT_PATH      "../objects/helmet.obj"
#define SCORE_TEXT_OBJECT_PATH  "../objects/scoreText.obj"
#define NUMBER_OBJECT_PATH      "../objects/numbers/"

#define NONTEXTURED_GEOMETRY_VERTEX_SHADER_PATH   "../shaders/coloredGeometry.vert"
#define NONTEXTURED_GEOMETRY_FRAGMENT_SHADER_PATH "../shaders/coloredGeometry.frag"
#define TEXTURED_GEOMETRY_VERTEX_SHADER_PATH      "../shaders/texturedGeometry.vert"
#define TEXTURED_GEOMETRY_FRAGMENT_SHADER_PATH    "../shaders/texturedGeometry.frag"

#define CALM_BACKGROUND_AUDIO_PATH                "../audio/Voyage_through_Wood_and_Sea.wav"
#define ARROW_FIRING_AUDIO_PATH                   "../audio/arrow_firing.wav"
#define ARROW_STRETCHING_AUDIO_PATH               "../audio/arrow_stretching.wav"

#define PLAYER_NOT_READY_TEXTURE_PATH             "../textures/PlayerNotReadyTexture.ppm"

#define HAND_SIZE                  0.1
#define HELMET_SIZE                0.4
#define SCORE_TEXT_SIZE            40
#define SCORE_DIGIT_SIZE           20
#define SCORE_DIGIT_SPACING        14
#define NOTIFICATION_SIZE          7

#define SYNC_RATE                  200
#define NANOSECONDS_IN_SECOND      1000000000

#define GRAVITY                    -9.81
#define ARROW_VELOCITY_SCALE       50.0

#define TOTAL_SINGLE_DIGIT         10
#define TOTAL_SCORE_DIGIT          9

static const float MAP_SIZE = 1000.0f;
static const float USER_HEIGHT = 1.676f;
static const float MAP_TRANSLATE_UP_OFFSET = MAP_SIZE * 0.0781f - USER_HEIGHT;

static const std::vector<glm::vec3> USER_TRANSLATION = {
    glm::vec3{ MAP_SIZE * -0.015f, MAP_SIZE * 0.015f + USER_HEIGHT, MAP_SIZE * -0.0008f },
    glm::vec3{ MAP_SIZE *  0.015f, MAP_SIZE * 0.015f + USER_HEIGHT, MAP_SIZE * -0.0008f }
};

static const std::vector<glm::vec3> NOTIFICATION_SCREEN_LOCATION = {
    glm::vec3(-20.0f, 16.5f, -5.8),
    glm::vec3( 20.0f, 16.5f, -5.8),
};

static const std::vector<glm::vec3> ARROW_STRING_LOCATION = {
    glm::vec3{ 0.03f,  0.4f, 0.1f },
    glm::vec3{ 0.03f, -0.4f, 0.1f }
};

static const glm::vec3 AIM_ASSIST_COLOR = { 1.0f, 0.65f, 0.0f };
static const glm::vec3 SCORE_TEXT_LOCATION = { 0.0f, 100.0f, -200.0f };
static const glm::vec3 SCORE_CENTER_LOCATION = { 0.0f, 75.0f, -200.0f };

class TowerDefender : public RiftApp
{
private:
    // Instance of game client to communicate with server
    std::unique_ptr<GameClient> gameClient;

    // Local copy of the server's game data
    rpcmsg::GameData gameData;
    rpcmsg::GameData previousGameData;
    std::mutex gameDataLock;
    uint32_t playerID;
    uint32_t playerTower;
    bool sessionActive;

    // Instance for audio play back
    std::unique_ptr<AudioPlayer> audioPlayer;

    // Relating to objects to render
    std::array<std::unique_ptr<OBJObject>, TOTAL_SINGLE_DIGIT> numberObject;
    std::unique_ptr<OBJObject> scoreTextObject;
    std::unique_ptr<OBJObject> environmentObject;
    std::unique_ptr<OBJObject> bowObject;
    std::unique_ptr<OBJObject> arrowObject;
    std::unique_ptr<OBJObject> sphereObject;
    std::unique_ptr<OBJObject> helmetObject;
    std::unique_ptr<Lines> lineObject;
    std::unique_ptr<Image> playerNotReadyImageObject;
    glm::mat4 environmentTransforms;
    GLuint nonTexturedShaderID, texturedShaderID;

    // Sync up with server
    void syncWithServer();

    // Helper functions
    rpcmsg::PlayerData getOculusPlayerState();
    void registerPlayer();
    std::vector<glm::mat4> getHandInformation();
    glm::mat4 getHeadInformation();

    void handleAudioUpdate(rpcmsg::GameData & currentGameData, rpcmsg::GameData & previousGameData);
    void renderScore(const glm::mat4 & projection, const glm::mat4 & headPose, rpcmsg::GameData & currentGameData);

protected:
    void initGl() override;
    void update() override;
    void shutdownGl() override;
    void renderScene(const glm::mat4 & projection, const glm::mat4 & headPose) override;


public:
    TowerDefender(std::string ipAddress, int portNumber);
    ~TowerDefender();
};