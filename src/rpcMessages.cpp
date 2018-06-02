#include "rpcMessages.hpp"

// Convert glm::vec2 over to an RPC message
rpcmsg::vec2 rpcmsg::glmToRPC(const glm::vec2 & data) {
	return rpcmsg::vec2{ data.x, data.y };
}

// Convert glm::vec3 over to an RPC message
rpcmsg::vec3 rpcmsg::glmToRPC(const glm::vec3 & data) {
	return rpcmsg::vec3{ data.x, data.y, data.z };
}

// Convert glm::vec4 over to an RPC message
rpcmsg::vec4 rpcmsg::glmToRPC(const glm::vec4 & data) {
	return rpcmsg::vec4{ data.x, data.y, data.z, data.w };
}

// Convert glm::mat4 over to an RPC message
rpcmsg::mat4 rpcmsg::glmToRPC(const glm::mat4 & data) {
	rpcmsg::mat4 result;
	for (int row = 0; row < 4; row++)
		for (int col = 0; col < 4; col++)
			result[row][col] = data[row][col];
	return result;
}

// Convert the RPC message's version of glm::vec2 back to glm::vec2
glm::vec2 rpcmsg::rpcToGLM(const rpcmsg::vec2 & data) {
	return glm::vec2(data.x, data.y);
}

// Convert the RPC message's version of glm::vec3 back to glm::vec3
glm::vec3 rpcmsg::rpcToGLM(const rpcmsg::vec3 & data) {
	return glm::vec3(data.x, data.y, data.z);
}

// Convert the RPC message's version of glm::vec4 back to glm::vec4
glm::vec4 rpcmsg::rpcToGLM(const rpcmsg::vec4 & data) {
	return glm::vec4(data.x, data.y, data.z, data.w);
}

// Convert the RPC message's version of glm::mat4 back to glm::mat4
glm::mat4 rpcmsg::rpcToGLM(const rpcmsg::mat4 & data) {
	glm::mat4 result;
	for (int row = 0; row < 4; row++)
		for (int col = 0; col < 4; col++)
			result[row][col] = data[row][col];
	return result;
}