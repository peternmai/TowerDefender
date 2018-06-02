#include <thread>
#include <chrono>
#include <memory>

#include "rpcMessages.hpp"

#define REFRESH_RATE           100
#define MILLISECONDS_IN_SECOND 1000
#define NANOSECONDS_IN_SECOND  1000000000

#define GRAVITY -9.81

#define ARROW_VELOCITY_SCALE       2.00
#define ARROW_RELOAD_ZONE_Z_OFFSET 0.30
#define ARROW_RELOAD_ZONE_RADIUS   0.15
#define ARROW_READY_ZONE_Z_OFFSET  0.30
#define ARROW_READY_ZONE_RADIUS    0.15
#define ARROW_DAMAGE               20

#define CASTLE_CRASHER_HIT_RADIUS  1.50

class GameEngine
{
private:

    rpcmsg::GameData gameData;
    std::unordered_map<uint32_t, rpcmsg::PlayerData> newPlayerData;

    std::chrono::nanoseconds lastUpdateTime;
    std::chrono::time_point<std::chrono::system_clock> start;
    std::chrono::nanoseconds sleepDuration;
    bool gameEngineServiceStatus;

    glm::vec3 calculateProjectileVelocity(
        const glm::vec3 & initVelocity, const uint64_t & launchTime);

    glm::vec3 calculateProjectilePosition(const glm::vec3 & initVelocity,
        const glm::vec3 & initPosition, const uint64_t & launchTime);

    void updateService();
    void updateProcedure();
    void updatePlayerData();
    void updateArrowData();
    void updateCastleCrasher();


public:
    GameEngine();
    ~GameEngine();

    rpcmsg::GameData getCopyOfGameData();
    void handleNewUserInput(uint32_t playerID, const rpcmsg::PlayerData & newInputs);
    void removeUser(uint32_t playerID);
};

