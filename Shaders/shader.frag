#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texcoord;
layout(location = 3) flat in int nodeInd;
layout(location = 4) in vec3 tangent;
layout(location = 5) in vec3 bitangent;
layout(location = 6) in vec3 toEnvLight;
layout(location = 7) in vec4 position;

struct Material {
	uint useValueAlbedo;
	uint useValueRoughness;
	uint useValueSpecular;
	uint useNormalMap;
	uint useDisplacementMap;
	int normalMap;
	int displacementMap;
	int albedoTexture;
	float roughness;
	int roughnessTexture;
	float specular;
	int type;
	// 0 - none, 1 - PBR, 2 - LAM, 3 - MIR, 4 - ENV, 5 - SIM
	// none is the same as simple
	int specularTexture;
	float albedor;
	float albedog;
	float albedob;
};
layout(binding = 2) uniform MaterialArray {
	Material arr[1000];
} materials;
layout(binding = 3) uniform sampler2D textures[100];
layout(binding = 4) uniform samplerCube cubes[100];
layout(binding = 5) uniform sampler2D lut;
struct Light {

	int type;
	// 0 none, 1 sun, 2 sphere, 3 spot
	float tintR;
	float tintG;
	float tintB;
	float angle;
	float strength;
	float radius;
	float power;
	float limit;
	float fov;
	float blend;
	int shadowRes;
};

layout(binding = 6) uniform LightTransforms {
    mat4 arr[1000];
} lightTransforms;

layout(binding = 7) uniform LightArray {
	Light arr[1000];
} lights;
struct LightNumInt
{
    int lightNumInt;
};
layout(binding = 8) uniform LightNum {
    LightNumInt inLightNum;
};


layout(location = 0) out vec4 outColor;

void main() {
	vec3 inLights[100];
	int numLights = inLightNum.lightNumInt;
	for(int lightInd = 0; lightInd < numLights; lightInd++){
        inLights[lightInd] = -(lightTransforms.arr[lightInd] * position).xyz;
    }
	Material material = materials.arr[nodeInd];
	vec3 useNormal = normal;
	//https://learnopengl.com/Advanced-Lighting/Normal-Mapping
	mat3 tbn = mat3(tangent,bitangent,useNormal);
	if(material.useNormalMap != 0){
		useNormal = 2 * texture(textures[material.normalMap], texcoord).xyz + vec3(1);
		useNormal = normalize(tbn * useNormal);
	}
    vec3 light = mix(vec3(0,0,0),vec3(1,1,1),0.75 + 0.25*dot(useNormal,vec3(0,0,1)));
	
	float normDot = dot(useNormal,vec3(0,0,1));
	if(material.type == 2){ //Diffuse
		vec3 directLight = vec3(0,0,0);
		for(int lightInd = 0; lightInd < numLights; lightInd++){
			Light light = lights.arr[lightInd];
			vec3 tint = vec3(light.tintR, light.tintG, light.tintB);
			float dist = length(inLights[lightInd]);
			float fallOff = max(0,1 - pow(dist/light.limit,4))/4/3.14159/dist/dist;
			vec3 sphereContribution = vec3(light.power*fallOff)*tint;
			if(light.type == 1){
				directLight += sphereContribution;
			}
			else if(light.type == 2){
				directLight += normDot*light.strength*tint;
			}
			else if(light.type == 3){
				float angle = acos(dot(normalize(inLights[lightInd]),vec3(0,0,1)));
				float blendLimit = light.fov*(1 - light.blend)/2;
				float fovLimit = light.fov/2;
				if(light.limit > dist){
					if(angle < blendLimit){
						directLight += normDot*sphereContribution;
					}
					else if(angle < fovLimit){
						float midPoint = angle - blendLimit;
						float blendFactor = (1- midPoint/(fovLimit - blendLimit));
						directLight += normDot*vec3(blendFactor) * sphereContribution;
					}
				}
			}
		}

		vec3 albedo;
		if(material.useValueAlbedo != 0){
			albedo = vec3(material.albedor,material.albedog,material.albedob);
		}
		else{
			vec4 rgbe = texture(cubes[material.albedoTexture], useNormal);
			int e = int(rgbe.w*255);
			albedo.x = ldexp((255*rgbe.x + 0.5)/256,e - 128);
			albedo.y = ldexp((255*rgbe.y + 0.5)/256,e - 128);
			albedo.z = ldexp((255*rgbe.z + 0.5)/256,e - 128);
		}
		outColor = vec4((directLight) * albedo * fragColor, 1.0);
	}
	else if(material.type == 3 || material.type == 4){
		outColor = vec4(fragColor, 1.0);
	}
	else if(material.type == 1){ //PBR
	
		vec3 albedo;
		if(material.useValueAlbedo != 0){
			albedo = vec3(material.albedor,material.albedog,material.albedob);
		}
		else{
			albedo = texture(textures[material.albedoTexture], texcoord).rgb;
		}
		outColor = vec4(albedo,1);
	}
	else{
	//None and simple - also currently PBR
		outColor = vec4(light * fragColor, 1.0);
	}
}