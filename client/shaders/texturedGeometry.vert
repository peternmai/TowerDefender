#version 330 core

layout (location = 0) in vec3 Position;
layout (location = 1) in vec2 UV_Coordinate;

uniform mat4 ModelMatrix;
uniform mat4 ProjectionMatrix;
uniform mat4 CameraMatrix;

out vec2 UV;

void main(void) {
   mat4 ModelView = CameraMatrix * ModelMatrix;
   gl_Position = ProjectionMatrix * ModelView * vec4(Position, 1.0f);

   // Sending information over to fragment shader
   UV = UV_Coordinate;
}