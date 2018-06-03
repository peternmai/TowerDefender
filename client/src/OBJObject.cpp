#include "OBJObject.hpp"
#include <vector>
#include <limits>
#include <cmath>


OBJObject::OBJObject(std::string filename, float size)
{
    // Attempt to parse the object file
    std::cout << "Loading in " << filename << "...";
    this->filename = filename;
    this->objectSize = size;
    this->assimpScene = loadModel(filename);
    std::cout << " successful" << std::endl;
}


OBJObject::~OBJObject()
{
    // Deallocate everything stored on GPU
    std::cout << "Deallocating " << filename << "...";
    for (int i = 0; i < this->meshes.size(); i++) {
        glDeleteVertexArrays(1, &this->meshes[i].VAO);
        glDeleteBuffers(1, &this->meshes[i].VBO);
        glDeleteBuffers(1, &this->meshes[i].EBO);
    }
    std::cout << "Successful" << std::endl;
}

// Read in the object file and load the content over to the GPU
// Make sure to center and resize the object
const aiScene * OBJObject::loadModel(const std::string & filename) {

    // Create an instance of assimp importer and read in file
    Assimp::Importer importer;
    const aiScene * scene = importer.ReadFile(filename, aiProcess_Triangulate);

    // Ensure that everything loaded in properly
    if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode) {
        std::cerr << "Unable to read in \"" << filename << "\"" << std::endl;
        throw std::runtime_error("Unable to read object file");
    }

    // Metadata to keep resize and center object
    float max_X = -std::numeric_limits<float>::infinity();
    float max_Y = -std::numeric_limits<float>::infinity();
    float max_Z = -std::numeric_limits<float>::infinity();
    float min_X = std::numeric_limits<float>::infinity();
    float min_Y = std::numeric_limits<float>::infinity();
    float min_Z = std::numeric_limits<float>::infinity();

    // Load each of the vertex attributes over to the GPU
    for (unsigned int meshIndex = 0; meshIndex < scene->mNumMeshes; meshIndex++) {
        meshData newMeshObject;

        // Decide side of each vertex attributes
        int sizeofVertexAttributes = 9;

        // Retrieve the indices to draw each of the element
        std::vector<unsigned int> drawIndices;
        for (unsigned int faceIndex = 0; faceIndex < scene->mMeshes[meshIndex]->mNumFaces; faceIndex++) {
            for (unsigned int index = 0; index < scene->mMeshes[meshIndex]->mFaces[faceIndex].mNumIndices; index++) {
                drawIndices.push_back(scene->mMeshes[meshIndex]->mFaces[faceIndex].mIndices[index]);
            }
        }

        // Assemble the vertex attributes (position, normal, etc)
        std::vector<glm::vec3> vertexAttributes;
        for (unsigned int index = 0; index < scene->mMeshes[meshIndex]->mNumVertices; index++) {

            glm::vec3 position = glm::vec3(
                (GLfloat)scene->mMeshes[meshIndex]->mVertices[index].x,
                (GLfloat)scene->mMeshes[meshIndex]->mVertices[index].y,
                (GLfloat)scene->mMeshes[meshIndex]->mVertices[index].z);
            vertexAttributes.push_back(position);

            glm::vec3 normal = glm::vec3(
                (GLfloat)scene->mMeshes[meshIndex]->mNormals[index].x,
                (GLfloat)scene->mMeshes[meshIndex]->mNormals[index].y,
                (GLfloat)scene->mMeshes[meshIndex]->mNormals[index].z);
            vertexAttributes.push_back(normal);

            unsigned int materialID = scene->mMeshes[meshIndex]->mMaterialIndex;
            glm::vec3 color = glm::vec3(1.0f);
            aiColor3D assimpColor;
            if((materialID < scene->mNumMaterials) && (AI_SUCCESS == scene->mMaterials[materialID]->Get(AI_MATKEY_COLOR_DIFFUSE, assimpColor)))
                color = glm::vec3(assimpColor.r, assimpColor.g, assimpColor.b);
            //std::cout << scene->mNumMaterials << " " << color.x << " " << color.y << " " << color.z << std::endl;
            vertexAttributes.push_back(color);

            // Update metadata
            min_X = std::min(min_X, position.x);
            min_Y = std::min(min_Y, position.y);
            min_Z = std::min(min_Z, position.z);
            max_X = std::max(max_X, position.x);
            max_Y = std::max(max_Y, position.y);
            max_Z = std::max(max_Z, position.z);
        }

        // Create Vertex Array Object (VAO)
        glGenVertexArrays(1, &newMeshObject.VAO);
        glBindVertexArray(newMeshObject.VAO);

        // Send Vertex Buffer Object / Attributes (VBO)
        glGenBuffers(1, &newMeshObject.VBO);
        glBindBuffer(GL_ARRAY_BUFFER, newMeshObject.VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertexAttributes[0]) * vertexAttributes.size(),
            &vertexAttributes[0], GL_STATIC_DRAW);

        // Set position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeofVertexAttributes * sizeof(GLfloat), (GLvoid*)0);
        glEnableVertexAttribArray(0);

        // Set normal attribute
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeofVertexAttributes * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
        glEnableVertexAttribArray(1);

        // Set color attribute
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FLOAT, sizeofVertexAttributes * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
        glEnableVertexAttribArray(2);

        // Specify order to draw (EBO)
        glGenBuffers(1, &newMeshObject.EBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, newMeshObject.EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(drawIndices[0]) * drawIndices.size(), &drawIndices[0], GL_STATIC_DRAW);
        newMeshObject.numberOfVerticesToDraw = (GLuint) drawIndices.size();

        // Unbind everything
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        // Keep track of this mesh
        this->meshes.push_back(newMeshObject);
    }

    // Determine how to shift to origin
    glm::vec3 shiftAmount;
    shiftAmount.x = (max_X + min_X) / 2 * -1;
    shiftAmount.y = (max_Y + min_Y) / 2 * -1;
    shiftAmount.z = (max_Z + min_Z) / 2 * -1;

    // Determine how much to scale
    float longestSide = std::max(max_X - min_X, std::max(max_Y - min_Y, max_Z - min_Z));
    float scaleAmount = this->objectSize / longestSide;

    this->centerAndScaleTransformation = glm::mat4(1.0f);
    this->centerAndScaleTransformation = glm::translate(glm::mat4(1.0f), shiftAmount) * this->centerAndScaleTransformation;
    this->centerAndScaleTransformation = glm::scale(glm::mat4(1.0f), glm::vec3(scaleAmount, scaleAmount, scaleAmount)) * this->centerAndScaleTransformation;

    // Successful, return the assimp scene graph
    return scene;
}

void OBJObject::draw(GLuint shaderProgram, const glm::mat4 & projection,
    const glm::mat4 & headPose, const glm::mat4 & transforms) {

    // Shift and scale object to center. Apply specified transformation
    glm::mat4 toWorld = this->centerAndScaleTransformation;
    toWorld = transforms * toWorld;

    // Render all of the meshes
    // Assimp nodes not working... drawing meshes directly instead...
    for (unsigned int i = 0; i < this->meshes.size(); i++) {

        // Use the specify shader
        glUseProgram(shaderProgram);

        // Get uniform location to send local variable to shader
        GLuint uModel = glGetUniformLocation(shaderProgram, "ModelMatrix");
        GLuint uProjection = glGetUniformLocation(shaderProgram, "ProjectionMatrix");
        GLuint uCamera = glGetUniformLocation(shaderProgram, "CameraMatrix");

        // Send variable
        glUniformMatrix4fv(uModel, 1, GL_FALSE, &toWorld[0][0]);
        glUniformMatrix4fv(uProjection, 1, GL_FALSE, &projection[0][0]);
        glUniformMatrix4fv(uCamera, 1, GL_FALSE, &headPose[0][0]);

        // Draw the mesh
        glBindVertexArray(this->meshes[i].VAO);
        glDrawElements(GL_TRIANGLES, this->meshes[i].numberOfVerticesToDraw, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
}

