#pragma once

#include <chrono>
#include <mutex>
#include <array>
#include <queue>
#include <cmath>

#include "MinimalOculus.hpp"
#include "GameClient.hpp"
#include "AudioPlayer.hpp"
#include "OBJObject.hpp"
#include "Lines.hpp"
#include "Image.hpp"
#include "shader.hpp"
#include "rpcMessages.hpp"

#define ENVIRONMENT_OBJECT_PATH                   "../objects/environment.obj"
#define BOW_OBJECT_PATH                           "../objects/bow.obj"
#define ARROW_OBJECT_PATH                         "../objects/arrow.obj"
#define SPHERE_OBJECT_PATH                        "../objects/sphere.obj"
#define HELMET_OBJECT_PATH                        "../objects/helmet.obj"
#define SCORE_TEXT_OBJECT_PATH                    "../objects/scoreText.obj"
#define NUMBER_OBJECT_PATH                        "../objects/numbers/"
#define CASTLE_CRASHER_PATH                       "../objects/castleCrashers/"
#define COMBO_MULTIPLIER_PATH                     "../objects/combo/"

#define CASTLE_CRASHER_BODY_NAME                  "body.obj"
#define CASTLE_CRASHER_LEFT_ARM_NAME              "leftArm.obj"
#define CASTLE_CRASHER_RIGHT_ARM_NAME             "rightArm.obj"
#define CASTLE_CRASHER_LEFT_LEG_NAME              "leftLeg.obj"
#define CASTLE_CRASHER_RIGHT_LEG_NAME             "rightLeg.obj"

#define NONTEXTURED_GEOMETRY_VERTEX_SHADER_PATH   "../shaders/coloredGeometry.vert"
#define NONTEXTURED_GEOMETRY_FRAGMENT_SHADER_PATH "../shaders/coloredGeometry.frag"
#define TEXTURED_GEOMETRY_VERTEX_SHADER_PATH      "../shaders/texturedGeometry.vert"
#define TEXTURED_GEOMETRY_FRAGMENT_SHADER_PATH    "../shaders/texturedGeometry.frag"

#define CALM_BACKGROUND_AUDIO_PATH                "../audio/Voyage_through_Wood_and_Sea.wav"
#define ARROW_FIRING_AUDIO_PATH                   "../audio/arrow_firing.wav"
#define ARROW_STRETCHING_AUDIO_PATH               "../audio/arrow_stretching.wav"
#define CONFIRMATION_AUDIO_PATH                   "../audio/confirmation.wav"

#define PLAYER_NOT_READY_TEXTURE_PATH             "../textures/PlayerNotReadyTexture.ppm"

#define M_PI                       3.14159265358979323846

#define HAND_SIZE                  0.1
#define HELMET_SIZE                0.4
#define SCORE_TEXT_SIZE            40
#define SCORE_DIGIT_SIZE           20
#define SCORE_DIGIT_SPACING        14
#define NOTIFICATION_SIZE          7
#define HEALTH_BAR_SIZE            200
#define HEALTH_BAR_WIDTH           20
#define MAX_MULTIPLIER             16
#define LOG2_16                    4
#define MULTIPLIER_TEXT_COMBO_SIZE 4

#define SYNC_RATE                  200
#define NANOSECONDS_IN_SECOND      1000000000

#define GRAVITY                    -9.81
#define ARROW_VELOCITY_SCALE       50.0

#define TOTAL_SINGLE_DIGIT         10
#define TOTAL_SCORE_DIGIT          9

#define DIFFERENT_CASTLE_CRASHER   4

static const float MAP_SIZE = 1000.0f;
static const float USER_HEIGHT = 1.8f;
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
    glm::vec3{ 0.0f,  0.4f, 0.1f },
    glm::vec3{ 0.0f, -0.4f, 0.1f }
};

static const glm::vec3 CASTLE_HEALTH_BAR_START_LOCATION = glm::vec3(10.0f, 15.0f, 20.0f);
static const glm::vec3 AIM_ASSIST_COLOR = { 1.0f, 0.65f, 0.0f };
static const glm::vec3 SCORE_TEXT_LOCATION = { 0.0f, 100.0f, -200.0f };
static const glm::vec3 SCORE_CENTER_LOCATION = { 0.0f, 75.0f, -200.0f };

// How much to scale each part of castle crasher
static const std::array<float, DIFFERENT_CASTLE_CRASHER> CASTLE_CRASHER_BODY_SIZE = { 2.0f, 2.0f, 2.0f, 2.0f };
static const std::array<float, DIFFERENT_CASTLE_CRASHER> CASTLE_CRASHER_LARM_SIZE = { 1.3f, 1.3f, 1.0f, 1.0f };
static const std::array<float, DIFFERENT_CASTLE_CRASHER> CASTLE_CRASHER_RARM_SIZE = { 1.5f, 3.0f, 1.5f, 1.5f };
static const std::array<float, DIFFERENT_CASTLE_CRASHER> CASTLE_CRASHER_LEG_SIZE = { 0.5f, 0.5f, 0.5f, 0.5f };

// Offset prior to rotation
static const std::array<glm::vec3, DIFFERENT_CASTLE_CRASHER> CASTLE_CRASHER_BODY_PRE_ROTATION =
    { glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.0f) };
static const std::array<glm::vec3, DIFFERENT_CASTLE_CRASHER> CASTLE_CRASHER_LEFT_ARM_PRE_ROTATION =
    { glm::vec3(0.0f, -0.5f, 0.1f), glm::vec3(0.0f, -0.5f, 0.0f), glm::vec3(0.0f, -0.4f, 0.0f), glm::vec3(0.0f, -0.4f, 0.0f) };
static const std::array<glm::vec3, DIFFERENT_CASTLE_CRASHER> CASTLE_CRASHER_RIGHT_ARM_PRE_ROTATION =
    { glm::vec3(0.0f, -0.4f, 0.5f), glm::vec3(0.0f, -0.3f, 0.0f), glm::vec3(0.0f, -0.4f, 0.5f), glm::vec3(0.0f, -0.4f, 0.5f) };
static const std::array<glm::vec3, DIFFERENT_CASTLE_CRASHER> CASTLE_CRASHER_LEFT_LEG_PRE_ROTATION =
    { glm::vec3(0.0f, -0.2f, 0.0f), glm::vec3(0.0f, -0.2f, 0.0f), glm::vec3(0.0f, -0.2f, 0.0f), glm::vec3(0.0f, -0.2f, 0.0f) };
static const std::array<glm::vec3, DIFFERENT_CASTLE_CRASHER> CASTLE_CRASHER_RIGHT_LEG_PRE_ROTATION =
    { glm::vec3(0.0f, -0.2f, 0.0f), glm::vec3(0.0f, -0.2f, 0.0f), glm::vec3(0.0f, -0.2f, 0.0f), glm::vec3(0.0f, -0.2f, 0.0f) };

// Offset after rotation
static const std::array<glm::vec3, DIFFERENT_CASTLE_CRASHER> CASTLE_CRASHER_BODY_OFFSET = 
    { glm::vec3(2.0f, 2.0f, 0.0f), glm::vec3(2.0f, 2.0f, 0.0f), glm::vec3(2.0f, 2.0f, 0.0f), glm::vec3(2.0f, 2.0f, 0.0f) };
static const std::array<glm::vec3, DIFFERENT_CASTLE_CRASHER> CASTLE_CRASHER_LEFT_ARM_OFFSET = 
    { glm::vec3(0.6f, -0.3f, 0.1f), glm::vec3(0.6f, -0.4f, 0.2f), glm::vec3(0.6f, -0.2f, 0.0f), glm::vec3(0.6f, -0.2f, 0.0f) };
static const std::array<glm::vec3, DIFFERENT_CASTLE_CRASHER> CASTLE_CRASHER_RIGHT_ARM_OFFSET = 
    { glm::vec3(-0.55f, -0.2f, 0.0f), glm::vec3(-0.6f, -0.2f, 0.0f), glm::vec3(-0.6f, -0.3f, 0.0f), glm::vec3(-0.7f, -0.2f, 0.0f) };
static const std::array<glm::vec3, DIFFERENT_CASTLE_CRASHER> CASTLE_CRASHER_LEFT_LEG_OFFSET = 
    { glm::vec3(0.2f, -1.0f, 0.0f), glm::vec3(0.2f, -1.0f, 0.0f), glm::vec3(0.2f, -1.0f, 0.0f), glm::vec3(0.2f, -1.0f, 0.0f) };
static const std::array<glm::vec3, DIFFERENT_CASTLE_CRASHER> CASTLE_CRASHER_RIGHT_LEG_OFFSET = 
    { glm::vec3(-0.2f, -1.0f, 0.0f), glm::vec3(-0.2f, -1.0f, 0.0f), glm::vec3(-0.2f, -1.0f, 0.0f), glm::vec3(-0.2f, -1.0f, 0.0f) };


class TowerDefender : public RiftApp
{
private:

    struct CastleCrasherObject {
        std::unique_ptr<OBJObject> bodyObject;
        std::unique_ptr<OBJObject> leftArmObject;
        std::unique_ptr<OBJObject> rightArmObject;
        std::unique_ptr<OBJObject> leftLegObject;
        std::unique_ptr<OBJObject> rightLegObject;
    };

    // Instance of game client to communicate with server
    std::unique_ptr<GameClient> gameClient;

    // Local copy of the server's game data
    rpcmsg::GameData incomingGameData;        // Constantly synchronizes with server (mutex needed)
    rpcmsg::GameData previousLocalGameData;   // Local game loop copy (no mutex needed)
    rpcmsg::GameData currentLocalGameData;    // Local game loop copy (no mutex needed)
    std::mutex incomingGameDataLock;
    uint32_t playerID;
    uint32_t playerTower;
    bool sessionActive;

    // Instance for audio play back
    std::unique_ptr<AudioPlayer> audioPlayer;

    // Relating to objects to render
    std::array<std::unique_ptr<OBJObject>, TOTAL_SINGLE_DIGIT> numberObject;
    std::array<std::unique_ptr<OBJObject>, LOG2_16 + 1> comboObject;
    std::array<CastleCrasherObject, DIFFERENT_CASTLE_CRASHER> castleCrasherObject;
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

    // Debug information
    long long lastRenderedTime = 0;
    std::queue<double> averageFpsQueue;
    double averageFPS;
    float keke = 0;

    // Sync up with server
    void syncWithServer();

    // Helper functions
    rpcmsg::PlayerData getOculusPlayerState();
    void registerPlayer();
    std::vector<glm::mat4> getHandInformation();
    glm::mat4 getHeadInformation();

    // Render functions
    void handleAudioUpdate(rpcmsg::GameData & currentGameData, rpcmsg::GameData & previousGameData);
    void renderScore(const glm::mat4 & projection, const glm::mat4 & headPose, rpcmsg::GameData & gameDataInstance);
    void renderPlayers(const glm::mat4 & projection, const glm::mat4 & headPose, rpcmsg::GameData & gameDataInstance);
    void renderFlyingArrows(const glm::mat4 & projection, const glm::mat4 & headPose, rpcmsg::GameData & gameDataInstance);
    void renderCastleCrashers(const glm::mat4 & projection, const glm::mat4 & headPose, rpcmsg::GameData & gameDataInstance);
    void renderCastleHealth(const glm::mat4 & projection, const glm::mat4 & headPose, rpcmsg::GameData & gameDataInstance);
    void renderNotification(const glm::mat4 & projection, const glm::mat4 & headPose, rpcmsg::GameData & gameDataInstance);
    void renderComboText(const glm::mat4 & projection, const glm::mat4 & headPose, rpcmsg::GameData & gameDataInstance);


protected:
    void initGl() override;
    void update() override;
    void shutdownGl() override;
    void renderScene(const glm::mat4 & projection, const glm::mat4 & headPose) override;


public:
    TowerDefender(std::string ipAddress, int portNumber);
    ~TowerDefender();
};