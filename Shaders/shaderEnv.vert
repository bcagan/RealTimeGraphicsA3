#version 450


layout(binding = 0) uniform Transforms {
    mat4 arr[1000];
} transforms;
layout(binding = 1) uniform Camera {
    mat4 camera;
} camera;
layout(binding = 9) uniform NormalTransforms {
    mat4 arr[1000];
} normTransforms;
layout(binding = 10) uniform EnvironmentTransforms {
    mat4 arr[1000];
} envTransforms;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec2 inTexcoord;
layout(location = 4) in vec3 inColor;
layout(location = 5) in int inNode;


layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 normal;
layout(location = 2) out vec2 texcoord;
layout(location = 3) flat out int nodeInd;
layout(location = 4) out vec3 tangent;
layout(location = 5) out vec3 bitangent;
layout(location = 6) out vec3 toEnvLight;
layout(location = 7) out vec4 position;

void main() {
    vec4 worldPos = transforms.arr[inNode] * vec4(inPosition, 1.0);
    normal = (normTransforms.arr[inNode] * vec4(inNormal, 1.0)).xyz;
    gl_Position = camera.camera * worldPos;
    position = worldPos;
    fragColor = inColor;
    nodeInd = inNode;
    tangent = inTangent;
    bitangent = cross(inNormal,inTangent);
    texcoord = inTexcoord;
    toEnvLight = -(envTransforms.arr[inNode] * worldPos).xyz;
}