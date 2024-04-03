#version 450

layout (input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput renderPass;
layout(location = 0) out vec4 outColor;

//https://learnopengl.com/Advanced-Lighting/HDR
void main() {
      vec3 hdrResult = subpassLoad(renderPass).rgb;
      const float exposure = 1;
      const float gamma = 2.2;
      vec3 hdrMapped = pow(vec3(1.0) - exp(-hdrResult * exposure), vec3(1.0/gamma));
      outColor = vec4(hdrMapped,1);
 }