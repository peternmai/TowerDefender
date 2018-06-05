#include "Lines.hpp"
#include "shader.hpp"

Lines::Lines()
{
    // Initialize end points of line
    this->endPoints[0] = glm::vec3(0.0f);
    this->endPoints[1] = glm::vec3(10.0f);

    // Create a container to store the end point of the line
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // Create an object to store the vertices
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * this->endPoints.size(), &this->endPoints[0], GL_STATIC_DRAW);

    // Set the position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    // Unbind so we don't mess up anything else
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Initialize a shader program for this line
    this->shaderProgramID = LoadShaders(LINE_VERTEX_SHADER_PATH, LINE_FRAGMENT_SHADER_PATH);
}

Lines::~Lines()
{
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}

void Lines::drawLine(glm::vec3 startPoint, glm::vec3 endPoint, glm::vec3 color,
    const glm::mat4 & projection, const glm::mat4 & camera) {

    // Update the points
    this->endPoints[0] = startPoint;
    this->endPoints[1] = endPoint;
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * 3 * this->endPoints.size(), &this->endPoints[0]);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Specify shader program to use
    glUseProgram(this->shaderProgramID);

    // Get uniform variable location to send to GPU
    GLuint uProjection = glGetUniformLocation(this->shaderProgramID, "ProjectionMatrix");
    GLuint uCamera = glGetUniformLocation(this->shaderProgramID, "CameraMatrix");
    GLuint uColor = glGetUniformLocation(this->shaderProgramID, "color");

    // Send variables over to the GPU
    glUniformMatrix4fv(uProjection, 1, GL_FALSE, &projection[0][0]);
    glUniformMatrix4fv(uCamera, 1, GL_FALSE, &camera[0][0]);
    glUniform3fv(uColor, 1, &color[0]);

    // Make line bigger
    glEnable(GL_LINE_SMOOTH);
    glLineWidth(5.0f);

    // Draw out the line
    glBindVertexArray(VAO);
    glDrawArrays(GL_LINES, 0, this->endPoints.size());
    glBindVertexArray(0);
}

