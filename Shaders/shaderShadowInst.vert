#version 450


layout(binding = 0) uniform Models {
    mat4 arr[1000];
} models;
struct PushConstants
{
    mat4 light;
};
layout( push_constant ) uniform PushConsts
{
	PushConstants inConsts;
};


layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec2 inTexcoord;
layout(location = 4) in vec3 inColor;
layout(location = 5) in int inNode;



void main() {
    vec4 worldPos = models.arr[inNode] * vec4(inPosition, 1.0);
    gl_Position = inConsts.light * worldPos;
}