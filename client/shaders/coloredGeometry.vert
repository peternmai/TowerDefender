#version 330 core

layout (location = 0) in vec3 Position;
layout (location = 1) in vec3 Normal;
layout (location = 2) in vec3 Color;

uniform mat4 ModelMatrix;
uniform mat4 ProjectionMatrix;
uniform mat4 CameraMatrix;

out vec3 vertNormal;
out vec3 vertPosition;
out vec3 vertColor;

void main(void) {
   mat4 ModelView = CameraMatrix * ModelMatrix;
   gl_Position = ProjectionMatrix * ModelView * vec4(Position, 1.0f);

   // Sending information over to fragment shader
   vertNormal = normalize(vec3(transpose(inverse(ModelMatrix)) * vec4(Normal, 0.0f)));
   vertPosition = vec3(ModelMatrix * vec4(Position, 1.0f));
   vertColor = Color;
}