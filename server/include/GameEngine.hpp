#include <thread>
#include <chrono>
#include <memory>
#include <mutex>

#include "rpcMessages.hpp"

#define REFRESH_RATE           200
#define MILLISECONDS_IN_SECOND 1000
#define NANOSECONDS_IN_SECOND  1000000000

#define GRAVITY -9.81
#define M_PI    3.14159265358979323846

#define ARROW_VELOCITY_SCALE       50.0
#define ARROW_RELOAD_ZONE_Z_OFFSET 0.30
#define ARROW_RELOAD_ZONE_RADIUS   0.30
#define ARROW_READY_ZONE_Z_OFFSET  0.25
#define ARROW_READY_ZONE_RADIUS    0.25
#define ARROW_DAMAGE               20
#define READY_UP_RADIUS            0.5

#define CASTLE_CRASHER_HIT_RADIUS  1.50

static const glm::vec3 ARROW_POSITION_OFFSET = glm::vec3{ -0.0f, 0.0f, -0.4f };

static const std::vector<glm::vec3> NOTIFICATION_SCREEN_LOCATION = {
    glm::vec3(-20.0f, 16.5f, -5.8),
    glm::vec3(20.0f, 16.5f, -5.8),
};

class GameEngine
{
private:

    rpcmsg::GameData gameData;
    std::mutex gameDataLock;
    std::unordered_map<uint32_t, rpcmsg::PlayerData> newPlayerData;

    std::chrono::nanoseconds lastUpdateTime;
    std::chrono::time_point<std::chrono::system_clock> start;
    std::chrono::nanoseconds sleepDuration;
    uint32_t easterEggLastUpdateTimer;
    bool gameEngineServiceStatus;

    glm::vec3 calculateProjectileVelocity(
        const glm::vec3 & initVelocity, const uint64_t & launchTime);

    glm::vec3 calculateProjectilePosition(const glm::vec3 & initVelocity,
        const glm::vec3 & initPosition, const uint64_t & launchTime);

    glm::mat4 calculateFlyingArrowPose(const rpcmsg::ArrowData & arrowData);

    void updateService();
    void updateProcedure();
    rpcmsg::GameData updatePlayerData(const rpcmsg::GameData & previousGameData);
    rpcmsg::GameData updateArrowData(const rpcmsg::GameData & previousGameData);
    rpcmsg::GameData updateCastleCrasher(const rpcmsg::GameData & previousGameData);
    rpcmsg::GameData updateGameState(const rpcmsg::GameData & previousGameData);
    rpcmsg::GameData updateEasterEgg(const rpcmsg::GameData & previousGameData);


public:
    GameEngine();
    ~GameEngine();

    rpcmsg::GameData getCopyOfGameData();
    void handleNewUserInput(uint32_t playerID, const rpcmsg::PlayerData & newInputs);
    void removeUser(uint32_t playerID);
};

