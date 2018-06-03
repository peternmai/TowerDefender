#version 330 core

layout (location = 0) in vec3 Position;

uniform mat4 ProjectionMatrix;
uniform mat4 CameraMatrix;

void main(void) {
   gl_Position = ProjectionMatrix * CameraMatrix * vec4(Position, 1.0f);
}