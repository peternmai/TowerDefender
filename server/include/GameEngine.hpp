#include <thread>
#include <chrono>
#include <memory>
#include <mutex>

#include "rpcMessages.hpp"

#define REFRESH_RATE           400
#define MILLISECONDS_IN_SECOND 1000
#define MILLI_TO_NANOSECONDS   1000000LL
#define NANOSECONDS_IN_SECOND  1000000000LL

#define GRAVITY -9.81f
#define M_PI    3.14159265358979323846f

#define ARROW_VELOCITY_SCALE        50.0f
#define ARROW_RELOAD_ZONE_Z_OFFSET  0.30f
#define ARROW_RELOAD_ZONE_RADIUS    0.30f
#define ARROW_READY_ZONE_Z_OFFSET   0.25f
#define ARROW_READY_ZONE_RADIUS     0.25f
#define ARROW_DAMAGE                100.0f
#define READY_UP_RADIUS             0.5f

#define CASTLE_CRASHER_HIT_RADIUS   1.0f
#define MAX_DIFFICULTY_SECONDS      180
#define MAX_CASTLE_CRASHERS         75
#define ANIMATION_TIME_SECONDS      1.0f
#define CASTLE_CRASHER_WALK_SPEED   2.0f
#define CASTLE_CRASHER_ATTACK_SPEED 1.0f
#define CASTLE_CRASHER_DAMAGE       1.0f

#define CASTLE_CRASHER_MAX_X        60.0f
#define CASTLE_CRASHER_MIN_X       -55.0f
#define CASTLE_CRASHER_MAX_Z        20.0f
#define CASTLE_CRASHER_MIN_Z       -80.0f
#define SPAWN_Z_RANGE               20.0f

#define CHEST_MIN_X                -9.0f
#define CHEST_MAX_X                 9.0f
#define CHEST_Z                     12

#define COMBO_TIME_SECONDS          3
#define MAX_MULTIPLIER              16
#define BASE_POINTS_PER_HIT         200

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
    std::mutex newPlayerDataLock;

    // Game meta data kept on server only
    std::chrono::nanoseconds gameStartTime;
    std::chrono::nanoseconds lastSpawnTime;
    std::chrono::nanoseconds spawnCooldownTimer;
    std::chrono::nanoseconds lastHitTime;
    float comboMultiplier;

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

