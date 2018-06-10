#ifndef __RPC_MESSAGES__
#define __RPC_MESSAGES__

#include <vector>
#include <cstdint>

#include "rpc/msgpack.hpp"

#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext.hpp>

#define LEFT_HAND  0
#define RIGHT_HAND 1

/**
 * Custom RPC messages specificially made for the tower defender game. This includes
 * the user's Oculus poses and the current state of the game.
 */
namespace rpcmsg {

    // Remote procedure function call name
    const std::string UPDATE_PLAYER_DATA = "UPDATE_PLAYER_DATA";
    const std::string GET_GAME_DATA = "GET_GAME_DATA";
    const std::string REQUEST_SERVER_SESSION = "REQUEST_SERVER_SESSION";
    const std::string CLOSE_SERVER_SESSION = "CLOSE_SERVER_SESSION";

    // ERROR messages
    const std::string INVALID_USER = "INVALID_USER_SPECIFIED";
    const std::string MAX_USER_EXCEEDED = "MAX_USER_EXCEEDED";

    // RPC message for vec2
    struct vec2 {
        float x, y;
        MSGPACK_DEFINE_ARRAY(x, y);
    };

    // RPC message for vec3
    struct vec3 {
        float x, y, z;
        MSGPACK_DEFINE_ARRAY(x, y, z);
    };

    // RPC message for vec4
    struct vec4 {
        float x, y, z, w;
        MSGPACK_DEFINE_ARRAY(x, y, z, w);
    };

    // RPC message for  mat4
    typedef std::array<std::array<float, 4>, 4> mat4;

    // RPC message that holds current state of each user's hand
    struct HandData {
        rpcmsg::mat4 handPose;
        rpcmsg::vec2 thumbstickValue;
        uint32_t     buttonState;
        float        indexTriggerValue;
        float        handTriggerValue;
        MSGPACK_DEFINE_ARRAY(handPose, thumbstickValue, buttonState, indexTriggerValue, handTriggerValue);
    };

    // RPC message that holds current state of user's head
    struct HeadData {
        rpcmsg::mat4 headPose;
        MSGPACK_DEFINE_ARRAY(headPose);
    };

    // RPC message that holds data for user's arrow
    struct ArrowData {
        rpcmsg::mat4 arrowPose;
        uint32_t     arrowType;
        uint64_t     launchTimeMilliseconds;
        rpcmsg::vec3 initVelocity;
        rpcmsg::vec3 initPosition;
        rpcmsg::vec3 position;
        MSGPACK_DEFINE_ARRAY(arrowPose, arrowType, launchTimeMilliseconds, initVelocity, initPosition, position);
    };

    // RPC message that holds all the data relating to the user
    struct PlayerData {
        rpcmsg::HeadData                headData;
        std::array<rpcmsg::HandData, 2> handData;
        rpcmsg::ArrowData               arrowData;
        uint32_t                        dominantHand;
        uint32_t                        arrowFiringAudioCue;
        uint32_t                        arrowStretchingAudioCue;
        bool                            arrowReleased;
        bool                            arrowReadying;
        MSGPACK_DEFINE_ARRAY(headData, handData, arrowData, dominantHand, 
            arrowFiringAudioCue, arrowStretchingAudioCue, arrowReleased, arrowReadying);
    };

    // RPC message that holds all data relating to a single castle crasher
    struct CastleCrasherData {
        uint8_t      id;
        bool         alive;
        float        health;
        float        animationCycle;
        rpcmsg::vec3 direction;
        rpcmsg::vec3 position;
        rpcmsg::vec3 endPosition;
        uint32_t     nextDirectionChangeTimeMilliseconds;
        uint32_t     lastAttackTimeMilliseconds;
        MSGPACK_DEFINE_ARRAY(id, alive, health, animationCycle, direction, position, 
            endPosition, nextDirectionChangeTimeMilliseconds, lastAttackTimeMilliseconds);
    };

    // RPC message that holds the current game state
    struct GameState {
        bool     gameStarted;
        uint32_t gameScore;
        float    castleHealth;
        bool     leftTowerReady;
        bool     rightTowerReady;
        uint32_t enemyDiedCue;
        uint32_t scoreMultiplier;
        std::list<rpcmsg::CastleCrasherData> castleCrasherData;
        std::list<rpcmsg::ArrowData> flyingArrows;
        MSGPACK_DEFINE_ARRAY(gameStarted, gameScore, castleHealth, leftTowerReady, 
            rightTowerReady, enemyDiedCue, scoreMultiplier, castleCrasherData, flyingArrows);
    };

    // RPC message that holds a copy of the entire game state
    struct GameData {
        std::unordered_map<uint32_t, rpcmsg::PlayerData> playerData;
        rpcmsg::GameState gameState;
        MSGPACK_DEFINE_ARRAY(playerData, gameState);
    };

    // Convert glm::vec2 over to an RPC message
    rpcmsg::vec2 glmToRPC(const glm::vec2 & data);

    // Convert glm::vec3 over to an RPC message
    rpcmsg::vec3 glmToRPC(const glm::vec3 & data);

    // Convert glm::vec4 over to an RPC message
    rpcmsg::vec4 glmToRPC(const glm::vec4 & data);

    // Convert glm::mat4 over to an RPC message
    rpcmsg::mat4 glmToRPC(const glm::mat4 & data);

    // Convert the RPC message's version of glm::vec2 back to glm::vec2
    glm::vec2 rpcToGLM(const rpcmsg::vec2 & data);

    // Convert the RPC message's version of glm::vec3 back to glm::vec3
    glm::vec3 rpcToGLM(const rpcmsg::vec3 & data);

    // Convert the RPC message's version of glm::vec4 back to glm::vec4
    glm::vec4 rpcToGLM(const rpcmsg::vec4 & data);

    // Convert the RPC message's version of glm::mat4 back to glm::mat4
    glm::mat4 rpcToGLM(const rpcmsg::mat4 & data);
}

#endif