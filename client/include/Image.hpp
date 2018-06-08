#pragma once

#include <string>
#include <vector>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext.hpp>

class Image
{
private:

    // The default vertices of the image with a dimension of 1.0f x 1.0f
    const std::vector<glm::vec3> IMAGE_VERTICES{
        glm::vec3(-0.5f, -0.5f, 0.0f),
        glm::vec3( 0.5f,  0.5f, 0.0f),
        glm::vec3(-0.5f,  0.5f, 0.0f),
        glm::vec3(-0.5f, -0.5f, 0.0f),
        glm::vec3( 0.5f, -0.5f, 0.0f),
        glm::vec3( 0.5f,  0.5f, 0.0f),
    };

    // The UV mapping of the image
    const std::vector<glm::vec2> IMAGE_UV{
        glm::vec2(0.0f, 1.0f),
        glm::vec2(1.0f, 0.0f),
        glm::vec2(0.0f, 0.0f),
        glm::vec2(0.0f, 1.0f),
        glm::vec2(1.0f, 1.0f),
        glm::vec2(1.0f, 0.0f),
    };

    GLuint VAO, VBO;
    GLuint textureID;

    std::vector<unsigned char> loadPPM(const char* filename, int& width, int& height);


public:
    Image(const std::string & filename);
    ~Image();

    void draw(GLuint shaderProgram, const glm::mat4 & projection,
        const glm::mat4 & headPose, const glm::mat4 & transform);
};

