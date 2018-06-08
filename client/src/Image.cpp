#include "Image.hpp"

#include <sstream>
#include <iostream>


Image::Image(const std::string & filename)
{
    // Attempt to load in the PPM image file
    std::cout << "Loading in " << filename << "... ";
    int width, height;
    std::vector<unsigned char> textureRGB = this->loadPPM(filename.c_str(), width, height);
    if (textureRGB.size() == 0)
        throw std::runtime_error("Failed to read in texture file");

    // Create OpenGL texture
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, &textureRGB[0]);

    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

    // Unbind texture so we don't mess it up
    glBindTexture(GL_TEXTURE_2D, 0);

    // Scale width and height to fit in a 1x1 box
    float scaledWidth = width / (float)((width > height) ? width : height);
    float scaledHeight = height / (float)((width > height) ? width : height);

    // Generate vertices attributes (position and UV)
    std::vector<float> vertexAttributes;
    for (int i = 0; i < this->IMAGE_VERTICES.size(); i++) {
        vertexAttributes.push_back(this->IMAGE_VERTICES[i].x * scaledWidth);
        vertexAttributes.push_back(this->IMAGE_VERTICES[i].y * scaledHeight);
        vertexAttributes.push_back(this->IMAGE_VERTICES[i].z);
        vertexAttributes.push_back(this->IMAGE_UV[i].x);
        vertexAttributes.push_back(this->IMAGE_UV[i].y);
    }

    // Create container to store this screen of the image (VAO -- Vertex Array Object)
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // Copy vertices of the screen over to the GPU (VBO -- Vertex Buffer Object)
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertexAttributes.size(),
        &vertexAttributes[0], GL_STATIC_DRAW);

    // Set the attribute (vertex) -- vec3
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT) * 5, (GLvoid*)0);
    glEnableVertexAttribArray(0);

    // Set the attribute (UV) -- vec2
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT) * 5, (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    // Unbind everything
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    std::cout << "Successful" << std::endl;
}

Image::~Image()
{
    glDeleteTextures(1, &textureID);
    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);
}

//! Load a ppm file from disk.
// @input filename The location of the PPM file.  If the file is not found, an error message
//		will be printed and this function will return 0
// @input width This will be modified to contain the width of the loaded image, or 0 if file not found
// @input height This will be modified to contain the height of the loaded image, or 0 if file not found
//
// @return Returns the RGB pixel data as interleaved  of unsigned chars 
// (R0 G0 B0 R1 G1 B1 R2 G2 B2 .... etc) or 0 if an error ocured
std::vector<unsigned char> Image::loadPPM(const char* filename, int& width, int& height)
{
    const int BUFSIZE = 128;
    FILE* fp;
    size_t read;
    unsigned char* rawData;
    char buf[3][BUFSIZE];
    char* retval_fgets;
    errno_t err;

    if ((err = fopen_s(&fp, filename, "rb")) != 0)
    {
        std::cerr << "error reading ppm file, could not locate " << filename << std::endl;
        width = 0;
        height = 0;
        return std::vector<unsigned char>();
    }

    // Read magic number:
    retval_fgets = fgets(buf[0], BUFSIZE, fp);

    // Read width and height:
    do
    {
        retval_fgets = fgets(buf[0], BUFSIZE, fp);
    } while (buf[0][0] == '#');
    std::string text(buf[0]);
    std::istringstream ss(text);
    ss >> width >> height;

    // Read maxval:
    do
    {
        retval_fgets = fgets(buf[0], BUFSIZE, fp);
    } while (buf[0][0] == '#');

    // Read image data:
    rawData = new unsigned char[width * height * 3];
    read = fread(rawData, width * height * 3, 1, fp);
    fclose(fp);
    if (read != 1)
    {
        std::cerr << "error parsing ppm file, incomplete data" << std::endl;
        delete[] rawData;
        width = 0;
        height = 0;

        return std::vector<unsigned char>();
    }

    // Store the byte array in a vector and deallocate the byte array
    std::vector<unsigned char> data;
    size_t sizeOfData = width * height * 3 * sizeof(unsigned char);
    data.reserve(sizeOfData);
    std::copy(&rawData[0], &rawData[sizeOfData], std::back_inserter(data));
    delete[] rawData;
    return data;
}

void Image::draw(GLuint shaderProgram, const glm::mat4 & projection,
    const glm::mat4 & headPose, const glm::mat4 & transform)
{
    // Specify shader program
    glUseProgram(shaderProgram);

    // Get uniform location to send local variable to shader
    GLuint uModel = glGetUniformLocation(shaderProgram, "ModelMatrix");
    GLuint uProjection = glGetUniformLocation(shaderProgram, "ProjectionMatrix");
    GLuint uCamera = glGetUniformLocation(shaderProgram, "CameraMatrix");

    // Send variable
    glUniformMatrix4fv(uModel, 1, GL_FALSE, &transform[0][0]);
    glUniformMatrix4fv(uProjection, 1, GL_FALSE, &projection[0][0]);
    glUniformMatrix4fv(uCamera, 1, GL_FALSE, &headPose[0][0]);

    // Draw out image
    glBindVertexArray(VAO);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Unbind so we dn't change anything
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);
}
