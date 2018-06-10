#include <iostream>
#include <vector>


#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

class OBJObject
{
private:

    struct meshData {
        GLuint VAO, VBO, EBO;
        GLuint numberOfVerticesToDraw;
    };

    std::string filename;
    const aiScene * assimpScene;
    std::vector<meshData> meshes;

    float objectSize;
    int textureWidth, textureHeight;

    glm::mat4 centerAndScaleTransformation;

    const aiScene * loadModel(const std::string & filename);

public:
    OBJObject(std::string filename, float size);
    ~OBJObject();

    void draw(GLuint shaderProgram, const glm::mat4 & projection, const glm::mat4 & headPose, 
        const glm::mat4 & transforms, const float opacity = 1.0f);
};