#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texcoord;
layout(location = 3) flat in int nodeInd;
layout(location = 4) in vec3 tangent;
layout(location = 5) in vec3 bitangent;
layout(location = 6) in vec3 toEnvLight;
layout(location = 7) in vec4 position;



layout(location = 0) out vec4 outColor;



void main() {
}