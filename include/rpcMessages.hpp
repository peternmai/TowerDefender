#include <vector>
#include <cstdint>

#include "rpc/msgpack.hpp"

#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext.hpp>

/**
 * Custom RPC messages specificially made for the tower defender game. This includes
 * the user's Oculus poses and the current state of the game.
 */
namespace rpcmsg {

	// Remote procedure function call name
	static const std::string UPDATE_PLAYER_DATA = "UPDATE_PLAYER_DATA";
	static const std::string GET_GAME_DATA = "GET_GAME_DATA";
	static const std::string REQUEST_SERVER_SESSION = "REQUEST_SERVER_SESSION";
	static const std::string CLOSE_SERVER_SESSION = "CLOSE_SERVER_SESSION";

	// ERROR messages
	static const std::string INVALID_USER = "INVALID_USER_SPECIFIED";
	static const std::string MAX_USER_EXCEEDED = "MAX_USER_EXCEEDED";

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

	// RPC message that holds all the data relating to the user
	struct PlayerData {
		rpcmsg::HeadData                headData;
		std::array<rpcmsg::HandData, 2> handData;
		MSGPACK_DEFINE_ARRAY(headData, handData);
	};

	// RPC message that holds all data relating to a single castle crasher
	struct CastleCrasherData {
		bool         alive;
		float        health;
		rpcmsg::vec3 position;
		MSGPACK_DEFINE_ARRAY(alive, health, position);
	};

	// RPC message that holds the current game state
	struct GameState {
		bool     gameStarted;
		uint32_t gameScore;
		uint32_t castleHealth;
		std::vector<rpcmsg::CastleCrasherData> castleCrashers;
		MSGPACK_DEFINE_ARRAY(gameStarted, gameScore, castleHealth, castleCrashers);
	};

	// RPC message that holds a copy of the entire game state
	struct GameData {
		std::unordered_map<uint32_t, rpcmsg::PlayerData> playerData;
		rpcmsg::GameState gameState;
		MSGPACK_DEFINE_ARRAY(playerData, gameState);
	};

	// Convert glm::vec2 over to an RPC message
	static rpcmsg::vec2 glmToRPC(const glm::vec2 & data) {
		return rpcmsg::vec2{ data.x, data.y };
	}

	// Convert glm::vec3 over to an RPC message
	static rpcmsg::vec3 glmToRPC(const glm::vec3 & data) {
		return rpcmsg::vec3{ data.x, data.y, data.z };
	}

	// Convert glm::vec4 over to an RPC message
	static rpcmsg::vec4 glmToRPC(const glm::vec4 & data) {
		return rpcmsg::vec4{ data.x, data.y, data.z, data.w };
	}

	// Convert glm::mat4 over to an RPC message
	static rpcmsg::mat4 glmToRPC(const glm::mat4 & data) {
		rpcmsg::mat4 result;
		for (int row = 0; row < 4; row++)
			for (int col = 0; col < 4; col++)
				result[row][col] = data[row][col];
		return result;
	}

	// Convert the RPC message's version of glm::vec2 back to glm::vec2
	static glm::vec2 rpcToGLM(const rpcmsg::vec2 & data) {
		return glm::vec2(data.x, data.y);
	}

	// Convert the RPC message's version of glm::vec3 back to glm::vec3
	static glm::vec3 rpcToGLM(const rpcmsg::vec3 & data) {
		return glm::vec3(data.x, data.y, data.z);
	}

	// Convert the RPC message's version of glm::vec4 back to glm::vec4
	static glm::vec4 rpcToGLM(const rpcmsg::vec4 & data) {
		return glm::vec4(data.x, data.y, data.z, data.w);
	}

	// Convert the RPC message's version of glm::mat4 back to glm::mat4
	static glm::mat4 rpcToGLM(const rpcmsg::mat4 & data) {
		glm::mat4 result;
		for (int row = 0; row < 4; row++)
			for (int col = 0; col < 4; col++)
				result[row][col] = data[row][col];
		return result;
	}

}