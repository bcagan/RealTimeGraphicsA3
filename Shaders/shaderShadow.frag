#version 450

layout(location = 0) in vec4 position;
layout(location = 0) out vec4 outColor;



void main() {
	float depth = 1 - position.z;
	outColor = vec4(depth,depth,depth,1);
}