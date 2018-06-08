#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext.hpp>

#include <array>

#define LINE_VERTEX_SHADER_PATH "../shaders/line.vert"
#define LINE_FRAGMENT_SHADER_PATH "../shaders/line.frag"

class Lines
{
private:
    GLuint VAO, VBO;
    GLuint shaderProgramID;
    std::array<glm::vec3, 2> endPoints;

public:
    Lines();
    ~Lines();

    void drawLine(glm::vec3 startPoint, glm::vec3 endPoint, glm::vec3 color,
        const glm::mat4 & projection, const glm::mat4 & camera, float lineWidth = 5.0f);
};

