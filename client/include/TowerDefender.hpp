#pragma once

#include <chrono>
#include <mutex>

#include "MinimalOculus.hpp"
#include "GameClient.hpp"
#include "OBJObject.hpp"
#include "shader.hpp"
#include "rpcMessages.hpp"

#define ENVIRONMENT_OBJECT_PATH "../objects/environment.obj"
#define BOW_OBJECT_PATH         "../objects/bow.obj"
#define ARROW_OBJECT_PATH       "../objects/arrow.obj"
#define SPHERE_OBJECT_PATH      "../objects/sphere.obj"

#define NONTEXTURED_GEOMETRY_VERTEX_SHADER_PATH "../shaders/coloredGeometry.vert"
#define NONTEXTURED_GEOMETRY_FRAGMENT_SHADER_PATH "../shaders/coloredGeometry.frag"

#define SYNC_RATE             100
#define NANOSECONDS_IN_SECOND 1000000000

static const float MAP_SIZE = 1000.0f;
static const float USER_HEIGHT = 1.676f;
static const float MAP_TRANSLATE_UP_OFFSET = MAP_SIZE * 0.0781f - USER_HEIGHT;

static const std::vector<glm::vec3> USER_TRANSLATION = {
    glm::vec3{ MAP_SIZE * -0.015f, MAP_SIZE * 0.015f + USER_HEIGHT, MAP_SIZE * -0.0008f },
    glm::vec3{ MAP_SIZE *  0.015f, MAP_SIZE * 0.015f + USER_HEIGHT, MAP_SIZE * -0.0008f }
};

class TowerDefender : public RiftApp
{
private:
    // Instance of game client to communicate with server
    std::unique_ptr<GameClient> gameClient;

    // Local copy of the server's game data
    rpcmsg::GameData gameData;
    std::mutex gameDataLock;
    uint32_t playerID;
    uint32_t playerTower;
    bool sessionActive;

    // Relating to objects to render
    std::unique_ptr<OBJObject> environmentObject;
    std::unique_ptr<OBJObject> bowObject;
    std::unique_ptr<OBJObject> arrowObject;
    std::unique_ptr<OBJObject> sphereObject;
    glm::mat4 environmentTransforms;
    GLuint nonTexturedShaderID;

    // Sync up with server
    void syncWithServer();

    // Helper functions
    rpcmsg::PlayerData getOculusPlayerState();
    void registerPlayer();
    std::vector<glm::mat4> getHandInformation();
    glm::mat4 getHeadInformation();

protected:
    void initGl() override;
    void update() override;
    void shutdownGl() override;
    void renderScene(const glm::mat4 & projection, const glm::mat4 & headPose) override;


public:
    TowerDefender(std::string ipAddress, int portNumber);
    ~TowerDefender();
};