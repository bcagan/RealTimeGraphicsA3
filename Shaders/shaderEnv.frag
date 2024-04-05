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
layout(binding = 2) uniform MaterialArrat {
	Material arr[1000];
} materials;
layout(binding = 3) uniform sampler2D textures[100];
layout(binding = 4) uniform samplerCube cubes[100];
layout(binding = 5) uniform sampler2D lut;
layout(binding = 8) uniform sampler2D shadows[100];
layout(binding = 10) uniform samplerCube environmentTexture;
struct Light {

	int type;
	// 0 none, 1 sphere, 2 sun, 3 spot
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
layout(binding = 9) uniform LightPerspective {
    mat4 arr[1000];
} lightPerspective;
struct PushConstants
{
    int lightNum;
	float cameraPosX;
	float cameraPosY;
	float cameraPosZ;
	float pbrP;
};
layout( push_constant ) uniform PushConsts
{
	PushConstants inConsts;
};

layout(location = 0) out vec4 outColor;

//https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping
float getShadowContribution(vec4 lightSpacePos){
	vec3 projectedPos = lightSpacePos.xyz;
	projectedPos = projectedPos * 0.5 + 0.5;
	float sampledDepth = texture(shadows[0],projectedPos.xy).r;
	float realDepth = projectedPos.z;
	float shadow = realDepth - 0.0005 > sampledDepth ? 0.1 : 1.0;
	return shadow;
}

void main() {
	vec3 toLight[100];
	vec4 lightSpace[100];
	int numLights = inConsts.lightNum;
	for(int lightInd = 0; lightInd < numLights; lightInd++){
        lightSpace[lightInd] = lightPerspective.arr[lightInd]*position;
        toLight[lightInd] = -(lightTransforms.arr[lightInd] * position).xyz;
    }
	Material material = materials.arr[nodeInd];
	vec3 useNormal = normal;
	//https://learnopengl.com/Advanced-Lighting/Normal-Mapping
	mat3 tbn = mat3(tangent,bitangent,useNormal);
	if(material.useNormalMap != 0){
		useNormal = 2 * texture(textures[material.normalMap], texcoord).xyz + vec3(1);
		useNormal = normalize(tbn * useNormal);
	}
    vec3 light = mix(vec3(0,0,0),vec3(1,1,1),0.75 + 0.25*dot(useNormal,vec3(0,0,-1)));
	if(material.type == 2){ //Diffuse
		vec3 directLight = vec3(0,0,0);
		for(int lightInd = 0; lightInd < numLights; lightInd++){
			Light light = lights.arr[lightInd];
			vec3 tint = vec3(light.tintR, light.tintG, light.tintB);
			float dist = length(toLight[lightInd]);
			float fallOff;
			if(light.limit > 0) fallOff = max(0,1 - pow(dist/light.limit,4))/4/3.14159/dist/dist;
			else fallOff = 1/dist/dist/4/3.14159;
			vec3 sphereContribution = vec3(light.power)*tint*fallOff;
			float shadowContribution = getShadowContribution(lightSpace[lightInd]);

			if(light.type == 1){
				float normDot = dot(useNormal,normalize(toLight[lightInd]));
				if (normDot < 0) normDot = 0;
				directLight += normDot * sphereContribution;
			}
			else if(light.type == 2){
				float normDot = dot(useNormal,vec3(0,0,-1));
				if (normDot < 0) normDot = 1 + normDot;
				else normDot = 1;
				directLight += normDot*light.strength*tint;
			}
			else if(light.type == 3){
				float normDot = dot(useNormal,vec3(0,0,-1));
				if (normDot < 0) normDot = 0;
				float angle = acos(dot(normalize(toLight[lightInd]),vec3(0,0,-1)));
				float blendLimit = light.fov*(1 - light.blend)/2;
				float fovLimit = light.fov/2;
				if(light.limit > dist){
					if(angle < blendLimit){
						directLight += shadowContribution*normDot*sphereContribution;
					}
					else if(angle < fovLimit){
						float midPoint = angle - blendLimit;
						float blendFactor = (1- midPoint/(fovLimit - blendLimit));
						directLight += shadowContribution*normDot*vec3(blendFactor) * sphereContribution;
					}
				}
			}
		}

		vec3 albedo;
		if(material.useValueAlbedo != 0){
			albedo = vec3(material.albedor,material.albedog,material.albedob);
		}
		else{
			albedo = texture(textures[material.albedoTexture], texcoord).rgb;
		}
		outColor = vec4((directLight) * albedo * fragColor, 1.0);
	}
	else if(material.type == 1){
	
		vec3 albedo;
		if(material.useValueAlbedo != 0){
			albedo = vec3(material.albedor,material.albedog,material.albedob);
		}
		else{
			albedo = texture(textures[material.albedoTexture], texcoord).rgb;
		}
		float roughness; //Roughness is being ignored right now
		if(material.useValueRoughness != 0){
			roughness = material.roughness;
		}
		else{
			roughness = texture(textures[material.roughnessTexture], texcoord).r;
			roughness /= 255.f;
		}
		float specular;
		if(material.useValueSpecular != 0){
			specular = material.specular;
		}
		else{
			specular = texture(textures[material.specularTexture], texcoord).r;
			specular /= 255.f;
		}
		vec3 objtoEnvLight = useNormal;
		vec4 rgbe = texture(environmentTexture, objtoEnvLight);
		int e = int(rgbe.w*255);
		vec3 radiance;
		radiance.x = ldexp((255*rgbe.x + 0.5)/256,e - 128);
		radiance.y = ldexp((255*rgbe.y + 0.5)/256,e - 128);
		radiance.z = ldexp((255*rgbe.z + 0.5)/256,e - 128);
		vec2 AB = texture(lut,texcoord).rg;

		vec3 cameraPos = vec3(inConsts.cameraPosX,inConsts.cameraPosY,inConsts.cameraPosZ);
		vec3 directLight = vec3(0,0,0);
		for(int lightInd = 0; lightInd < numLights; lightInd++){
			Light light = lights.arr[lightInd];
			vec3 tint = vec3(light.tintR, light.tintG, light.tintB);
			vec3 r = reflect(cameraPos - position.xyz, useNormal);
			float p = inConsts.pbrP;
			float shadowContribution = getShadowContribution(lightSpace[lightInd]);

			if(light.type == 1){
				vec3 centerToRay = dot(r,toLight[lightInd])*r - toLight[lightInd];
				vec3 closestPoint = toLight[lightInd] + centerToRay*light.radius/length(centerToRay);
				float normDot = dot(useNormal,normalize(closestPoint));
				if (normDot < 0) normDot = 0;
				float phi = acos(dot(normalize(r),normalize(toLight[lightInd])));
				float dist = length(closestPoint);
				float alpha = roughness*roughness;
				float alphaP = alpha + light.radius/2/dist;
				float sphereNormalization = pow((alpha/alphaP),2);
				float fallOff;
				if(light.limit > 0) fallOff = max(0,1 - pow(dist/light.limit,4))/4/3.14159/dist/dist;
				else fallOff = 1/dist/dist/4/3.14159;
				vec3 sphereContribution = vec3((light.power + 2)/(2*3.14159)*pow(phi,p))*tint*fallOff;
				directLight += normDot*sphereNormalization*sphereContribution;
			}
			else if(light.type == 2){
				r = normalize(r);
				float angle = acos(dot(r,vec3(0,0,-1)));
				float solidAngle = 3.14159*(light.angle/2)*(light.angle/2);
				float normalizeAngle = solidAngle/4/3.14159;
				float phi = acos(dot(r,vec3(0,0,-1)));
				if(dot(useNormal,vec3(0,0,-1)) < 0){
					phi = 0;
				}
				else if(angle > light.angle / 2){
					phi = light.angle/2;
				}
				vec3 sphereContribution = pow(phi,p)*light.strength*tint;
				directLight += normalizeAngle * sphereContribution;
			}
			else if(light.type == 3){
				
				vec3 centerToRay = dot(r,toLight[lightInd])*r - toLight[lightInd];
				vec3 closestPoint = toLight[lightInd] + centerToRay*light.radius/length(centerToRay);
				float phi = acos(dot(normalize(r),normalize(toLight[lightInd])));
				float dist = length(closestPoint);
				float alpha = roughness*roughness;
				float alphaP = alpha + light.radius/2/dist;
				float sphereNormalization = pow((alpha/alphaP),2);
				float angle = acos(dot(normalize(closestPoint),vec3(0,0,-1)));
				float blendLimit = light.fov*(1 - light.blend)/2;
				float fovLimit = light.fov/2;
				float fallOff;
				if(light.limit > 0) fallOff = max(0,1 - pow(dist/light.limit,4))/4/3.14159/dist/dist;
				else fallOff = 1/dist/dist/4/3.14159;
				vec3 sphereContribution = shadowContribution*vec3((light.power + 2)/(2*3.14159)*pow(phi,p))*tint*fallOff;
				if(light.limit > dist){
					
					float normDot = dot(useNormal,vec3(0,0,-1));
					if (normDot < 0) normDot = 0;
					if(angle < blendLimit){
						directLight += normDot*sphereNormalization *sphereContribution;
					}
					else if(angle < fovLimit){
						float midPoint = angle - blendLimit;
						float blendFactor = (1- midPoint/(fovLimit - blendLimit));
						directLight += normDot*vec3(blendFactor)*sphereNormalization * sphereContribution;
					}
				}
			}
		}

		outColor = vec4(albedo * (directLight + specular * AB.x + vec3(AB.y,AB.y,AB.y)),1);
	}
	else if(material.type == 3 || material.type == 4){
		vec3 objtoEnvLight = useNormal;
		if(material.type == 3) objtoEnvLight = reflect(toEnvLight,useNormal);
		vec4 rgbe = texture(environmentTexture, objtoEnvLight);
		int e = int(rgbe.w*255);
		vec3 radiance;
		radiance.x = ldexp((255*rgbe.x + 0.5)/256,e - 128);
		radiance.y = ldexp((255*rgbe.y + 0.5)/256,e - 128);
		radiance.z = ldexp((255*rgbe.z + 0.5)/256,e - 128);
		outColor = vec4(radiance * fragColor, 1.0);
	}
	else{
	//None and simple - also currently PBR
		outColor = vec4(light * fragColor, 1.0);
	}
}