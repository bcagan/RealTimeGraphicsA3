#include "SceneGraph.h"
#include "MathHelpers.h"
#include <string>
#include <vector>
#include <optional>
#include "Vertex.h"
#include <vulkan/vulkan_core.h>
#include <stdexcept>
#include <algorithm>
#include <chrono>
#include <cassert>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"



Texture Texture::parseTexture(std::string path, bool cube) {
	Texture tex;
	tex.format = cube ? Texture::FORM_RGBE : Texture::FORM_LIN;
	tex.type = cube ? Texture::TYPE_CUBE : Texture::TYPE_2D;
	int x, y, n;
	stbi_uc* rawData = stbi_load(path.c_str(), &x, &y, &n, 4);
	if (!rawData) {
		if (stbi_failure_reason()) {
			throw std::runtime_error("ERROR: Unable to load texture at path " + path + " with error: " + stbi_failure_reason());
		}
		else {
			throw std::runtime_error("ERROR: Unable to load texture at path " + path + " with no stbi error");
		}
	}
	tex.x = x;
	tex.y = cube ? y / 6 : y;
	tex.realY = y;
	//Every surface of the cube has the same image resolution
	tex.data = rawData;
	//https://vulkan-tutorial.com/Generating_Mipmaps
	tex.mipLevels = std::floor(std::log2(std::max(x, tex.y))) + 1;
	return tex;
}

//Given a pool of vertices produce a simple bounding sphere: center and radius
std::pair<float_3, float> DrawNode::produceBoundingSphere(std::vector<Vertex> vertices) {
	//Find Mesh Center
	std::pair<float,float> xDomain = std::make_pair(INFINITY,-INFINITY);
	std::pair<float,float> yDomain = std::make_pair(INFINITY, -INFINITY);
	std::pair<float,float> zDomain = std::make_pair(INFINITY, -INFINITY);
	for (Vertex vert : vertices) {
		if (vert.pos.x < xDomain.first) xDomain.first = vert.pos.x;
		if (vert.pos.y < yDomain.first) yDomain.first = vert.pos.y;
		if (vert.pos.z < zDomain.first) zDomain.first = vert.pos.z;
	}
	for (Vertex vert : vertices) {
		if (vert.pos.x > xDomain.second) xDomain.second = vert.pos.x;
		if (vert.pos.y > yDomain.second) yDomain.second = vert.pos.y;
		if (vert.pos.z > zDomain.second) zDomain.second = vert.pos.z;
	}
	float_3 center = float_3(
		xDomain.first + (xDomain.second - xDomain.first) / 2.0,
		yDomain.first + (yDomain.second - yDomain.first) / 2.0,
		zDomain.first + (zDomain.second - zDomain.first) / 2.0
	);
	//Find radius
	float radius = 0;
	for (Vertex vert : vertices) {
		float_3 distVec = vert.pos - center;
		float dist = distVec.norm();
		if (dist > radius) radius = dist;
	}
	return std::make_pair(center, radius);
}

//The main recursive function that navigates the scene graphs. Paramaters are as follows
//cameras - a reference to a vector of all processed draw cameras found in the graph
//vertices - a reference to the new vertex pool
//indicies - a referece to the new index pool
//node - current node index
//toWorld - the recursively calculated PARENT transforms from the current node
//		space to world space
//fromWorld - the recursively calculated PARENT transforms from world space to
//		the current node space
//int& drawNode a current node will be the drawNode node found when 
//		recursively navigating the scene graph. ID of final draw list node
//nodeDrivers - a reference to the pool of all node drivers
//cameraDrivers - areference to the pool of all camera drivers
//instancedTransforms - For instanced mesh #x, directly put transform into
//		instanced transform pool, skipping the need to add to the draw list
//instancedNormalTransforms - Above but for nromals
//drawNodes - a reference to the final list of draw nodes. Using a reference
//		should eliminate "stack" size when not using instances
void SceneGraph::recurseSceneGraph(
	std::vector<DrawCamera>& drawCameras,
	std::vector<Light>& drawLights,
	std::vector<Vertex>& vertices, 
	std::vector<uint32_t> & indices,
	int node, 
	mat44<float> toWorld, 
	mat44<float> fromWorld, 
	int& drawNode, 
	std::vector<Driver>& nodeDrivers, 
	std::vector<Driver>& cameraDrivers,
	std::vector<std::vector<mat44<float>>>& instancedTransforms,
	std::vector<std::vector<mat44<float>>>& instancedNormalTransforms,
	std::vector<DrawNode>& drawNodes) {
	//Get graph node and related transform information
	if (node >= graphNodes.size()) {
		throw std::runtime_error("ERROR: invalid node index in Scene Graph.");
	}
	GraphNode graphNode = graphNodes[node];
	float_3 scale = graphNode.scale;
	float_3 translate = graphNode.translate;
	quaternion<float> rotation = graphNode.rotation;
	
	//Transform from local space to world space
	mat44 scaleMat = mat44<float>(scale.x, 0, 0, 0, 0, scale.y, 0, 0, 0, 0, scale.z, 0, 0, 0, 0, 1);
	mat44<float> rotScaled = quaternion<float>::rotate(scaleMat, rotation);
	rotScaled.data[3][0] = translate.x;
	rotScaled.data[3][1] = translate.y;
	rotScaled.data[3][2] = translate.z;
	mat44<float> local = rotScaled;
	mat44 localToWorld = toWorld * local;

	//Produce inverse matrix;
	mat44<float> invScaleMat = mat44<float>(1 / scale.x, 0, 0, 0, 0, 1 / scale.y, 0, 0, 0, 0, 1 / scale.z, 0, 0, 0, 0, 1);
	quaternion<float> invRotation = rotation.invert();
	mat44<float> invRotScaled = quaternion<float>::rotate(invScaleMat, invRotation);
	float_3 invTranslate = invRotScaled * (translate * -1);
	invRotScaled.data[3][0] = invTranslate.x;
	invRotScaled.data[3][1] = invTranslate.y;
	invRotScaled.data[3][2] = invTranslate.z;

	//Transform from world space to local space
	mat44 worldToLocal = invRotScaled * fromWorld;
	mat44 normalToWorld = worldToLocal.transpose();

	//Handle environment
	if (graphNode.hasEnvironment) {
		worldToEnvironment = worldToLocal;
		environmentToWorld = localToWorld;
	}

	if (graphNode.light.has_value()) {
		int lightInd = *graphNode.light;
		Light newLight = lights[lightInd];
		newLight.toLocal = worldToLocal;
		drawLights.push_back(newLight);
	}

	//Handle camera nodes
	if (graphNode.camera.has_value()) {
		//Load standard camera information
		DrawCamera drawCamera;
		Camera graphCamera = cameras[*graphNode.camera];
		drawCamera.name = graphCamera.name;
		Perspective persp = graphCamera.perspective;
		mat44<float> perspMat = mat44<float>::perspective(persp.vfov, persp.aspect, persp.nearP, persp.farP);
		drawCamera.perspective = perspMat;
		drawCamera.perspectiveInfo = persp;
		drawCamera.transform = worldToLocal;

		//For animation
		drawCamera.forAnimate.parent = fromWorld;
		drawCamera.forAnimate.translate = translate;
		drawCamera.forAnimate.rotation = rotation;
		drawCamera.forAnimate.scale;

		drawCameras.push_back(drawCamera);
		if (graphNode.rotateDriver.has_value()) {
			//Handle driver
			Driver newDriver = Driver::fromSceneDriver(*(graphNode.rotateDriver));
			newDriver.id = graphNode.index;
			cameraDrivers.push_back(newDriver);
		}
		if (graphNode.translateDriver.has_value()) {
			//Handle driver
			Driver newDriver = Driver::fromSceneDriver(*(graphNode.translateDriver));
			newDriver.id = graphNode.index;
			cameraDrivers.push_back(newDriver);
		}
		if (graphNode.scaleDriver.has_value()) {
			//Handle driver
			Driver newDriver = Driver::fromSceneDriver(*(graphNode.scaleDriver));
			newDriver.id = graphNode.index;
			cameraDrivers.push_back(newDriver);
		}
	}
	//Handle two possible mesh cases
	if (graphNode.mesh.has_value()) {
		Mesh mesh = meshes[*graphNode.mesh];
		//Handle driver independently of if node is instanced
		if (graphNode.scaleDriver.has_value()) {
			Driver newDriver = Driver::fromSceneDriver(*(graphNode.scaleDriver));
			newDriver.id = graphNode.index;
			nodeDrivers.push_back(newDriver);
		}
		if (graphNode.translateDriver.has_value()) {
			Driver newDriver = Driver::fromSceneDriver(*(graphNode.translateDriver));
			newDriver.id = graphNode.index;
			nodeDrivers.push_back(newDriver);
		}
		if (graphNode.rotateDriver.has_value()) {
			Driver newDriver = Driver::fromSceneDriver(*(graphNode.rotateDriver));
			newDriver.id = graphNode.index;
			nodeDrivers.push_back(newDriver);
		}
		//Parse a stanrdard draw node
		if (!mesh.instanceMesh || !useInstancing) {
			//Create draw node
			DrawNode draw;
			draw.name = graphNode.name;
			draw.transform = localToWorld;
			draw.normalTransform = normalToWorld;
			draw.forAnimate.translate = translate;
			draw.forAnimate.rotation = rotation;
			draw.forAnimate.scale = scale;
			draw.forAnimate.parent = toWorld;
			draw.count = mesh.count;
			draw.start = vertices.size();
			draw.indexStart = indices.size();
			if (mesh.material.has_value()) {
				draw.material = materials[*mesh.material];
			}

			//Important: Vertices AND indices need to be added in THE ORDER TRAVERSED for the later pooling code to work!
			//Load vertices and indicies into draw pools
			//Get vertices
			std::vector<SceneVertex> meshSceneVertices(vertexPool.begin() + mesh.vertexOffset, vertexPool.begin() + mesh.vertexOffset + draw.count);
			std::vector<Vertex> meshVertices(meshSceneVertices.size());
			for (size_t vert = 0; vert < meshSceneVertices.size(); vert++) {
				meshVertices[vert].color = meshSceneVertices[vert].color;
				meshVertices[vert].pos = meshSceneVertices[vert].pos;
				meshVertices[vert].normal = meshSceneVertices[vert].normal;
				meshVertices[vert].tangent = meshSceneVertices[vert].tangent;
				meshVertices[vert].texcoord = meshSceneVertices[vert].texcoord;
				meshVertices[vert].node = drawNode;
			}
			vertices.reserve(vertices.size() + meshVertices.size());
			vertices.insert(vertices.end(), meshVertices.begin(), meshVertices.end());
			draw.boundingSphere = draw.produceBoundingSphere(meshVertices);

			//Get indices
			std::vector<uint32_t> meshIndices = std::vector<uint32_t>(mesh.indicies.has_value() ? (uint32_t)mesh.indicies->size() : (uint32_t)mesh.count);
			if (mesh.indicies.has_value()) {

				for (int index = 0; index < mesh.indicies->size(); index++) {
					meshIndices[index] = (*mesh.indicies)[index] + draw.start;
				}
			}
			else {
				for (int index = 0; index < mesh.count; index++) {
					meshIndices[index] = draw.start + index;
				}
			}
			indices.reserve(indices.size() + meshIndices.size());
			indices.insert(indices.end(), meshIndices.begin(), meshIndices.end());
			draw.indexCount = indices.size() - draw.indexStart;
			//insert node into draw list
			drawNodes.push_back(draw);
			drawNode++;
		}
		else {
			//Instanced case is quite simple, just need to parse transform info
			int poolIndex = instancedToPool[mesh.name];
			instancedTransforms[poolIndex].push_back(localToWorld);
			instancedNormalTransforms[poolIndex].push_back(normalToWorld);
		}
	}
	//Recurse upon all children ids
	for (int child : graphNode.children) {
		recurseSceneGraph(
			drawCameras, 
			drawLights,
			vertices, 
			indices, 
			child, 
			localToWorld, 
			worldToLocal, 
			drawNode,
			nodeDrivers,
			cameraDrivers,
			instancedTransforms,
			instancedNormalTransforms,
			drawNodes);
	}
}


//Recurse scene graph from all root nodes and then return intermediate information
DrawListIntermediate SceneGraph::navigateSceneGraphInt(int instanceSize) {
	DrawListIntermediate drawList;
	drawList.vertexPool = std::vector<Vertex>();
	drawList.lights = std::vector<Light>();
	drawList.indexPool = std::vector<uint32_t>();
	drawList.cameras = std::vector<DrawCamera>();
	drawList.drawPool = std::vector<DrawNode>();
	drawList.cameraDrivers = std::vector<Driver>();
	drawList.nodeDrivers = std::vector<Driver>();
	drawList.instancedTransforms = std::vector < std::vector<mat44<float>>>(instanceSize);
	drawList.instancedNormalTransforms = std::vector < std::vector<mat44<float>>>(instanceSize);
	drawList.textureMaps = textureMaps;
	drawList.cubeMaps = cubeMaps;
	drawList.environmentMap = environmentMap;
	int drawNode = 0;
	for (int root : roots) {
		std::vector<DrawNode> rootList;
		recurseSceneGraph(
			drawList.cameras,
			drawList.lights,
			drawList.vertexPool,
			drawList.indexPool,
			root,
			mat44<float>(1),
			mat44<float>(1),
			drawNode,
			drawList.nodeDrivers,
			drawList.cameraDrivers,
			drawList.instancedTransforms,
			drawList.instancedNormalTransforms,
			rootList);
		drawList.drawPool.reserve(drawList.drawPool.size() + rootList.size());
		drawList.drawPool.insert(drawList.drawPool.end(), rootList.begin(), rootList.end());
	}
	drawList.transforms = std::vector<mat44<float>>();
	drawList.normalTransforms = std::vector<mat44<float>>();
	for (DrawNode node : drawList.drawPool) {
		drawList.transforms.push_back(node.transform);
		drawList.normalTransforms.push_back(node.normalTransform);
	}
	drawList.worldToEnvironment = worldToEnvironment;
	return drawList;
}

//Find which meshes should be instanced and which should use drawnodes
void SceneGraph::navigateSceneGraphMeshes(std::vector<bool>& encountered, int node) {
	if (graphNodes[node].mesh.has_value()) {
		int meshId = *graphNodes[node].mesh;
		if (encountered[meshId]) {
			meshes[meshId].instanceMesh = true;
		}
		else {
			encountered[meshId] = true;
		}
	}
	for (int child : graphNodes[node].children) {
		navigateSceneGraphMeshes(encountered, child);
	}
}


std::vector<DrawLight> SceneGraph::toDrawLights(std::vector<Light> lights) {
	std::vector<DrawLight> retLights;
	for (Light light : lights) {
		DrawLight drawLight;
		drawLight.angle = light.angle;
		drawLight.blend = light.blend;
		drawLight.fov = light.fov;
		drawLight.limit = light.limit;
		drawLight.power = light.power;
		drawLight.radius = light.radius;
		drawLight.shadowRes = light.shadowRes;
		drawLight.strength = light.strength;
		drawLight.tintB = light.tintB;
		drawLight.tintG = light.tintG;
		drawLight.tintR = light.tintR;
		drawLight.type = light.type;
		retLights.push_back(drawLight);
	}
	return retLights;
}


std::vector<DrawLight> SceneGraph::toDrawLights(std::vector<Light> lights, std::vector<mat44<float>>& transforms) {
	std::vector<DrawLight> retLights;
	for (Light light : lights) {
		DrawLight drawLight;
		drawLight.angle = light.angle;
		drawLight.blend = light.blend;
		drawLight.fov = light.fov;
		drawLight.limit = light.limit;
		drawLight.power = light.power;
		drawLight.radius = light.radius;
		drawLight.shadowRes = light.shadowRes;
		drawLight.strength = light.strength;
		drawLight.tintB = light.tintB;
		drawLight.tintG = light.tintG;
		drawLight.tintR = light.tintR;
		drawLight.type = light.type;
		retLights.push_back(drawLight);
		transforms.push_back(light.toLocal);
	}
	return retLights;

}

DrawMaterial processMaterial(std::optional<Material> matOpt) {
	DrawMaterial drawMat;
	if (matOpt.has_value()) {
		Material mat = *matOpt;
		drawMat.type = mat.type;
		if (mat.displacementMap.has_value()) {
			drawMat.useDisplacementMap = true;
			drawMat.displacementMap = *mat.displacementMap;
		}
		if (mat.normalMap.has_value()) {
			drawMat.useNormalMap = true;
			drawMat.normalMap = *mat.normalMap;
		}
		//Data
		drawMat.useValueAlbedo = mat.data.useValueAlbedo;
		drawMat.useValueRoughness = mat.data.useValueRoughness;
		drawMat.useValueSpecular = mat.data.useValueSpecular;
		if (drawMat.useValueAlbedo) {
			drawMat.albedor = mat.data.albedo.x;
			drawMat.albedog = mat.data.albedo.y;
			drawMat.albedob = mat.data.albedo.z;
		}
		else {
			drawMat.albedoTexture = mat.data.albedoTexture;
		}
		if (drawMat.useValueRoughness) {
			drawMat.roughness = mat.data.roughness;
		}
		else {
			drawMat.roughnessTexture = mat.data.roughnessTexture;
		}
		if (drawMat.useValueSpecular) {
			drawMat.specular = mat.data.specular;
		}
		else {
			drawMat.specularTexture = mat.data.specularTexture;
		}
	}
	else {
		drawMat.type = MAT_NONE;
	}
	return drawMat;
}

DrawList SceneGraph::navigateSceneGraph(bool verbose, int poolSize) {
	if (poolSize > maxPool) {
		throw std::runtime_error("ERROR: pool size requested by the user is larger than the maximum definable in a SceneGraph. " + maxPool);
	}
	std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
	DrawList list;
	list.useInstancing = useInstancing;
	std::vector<std::pair<std::string, int>> instancedMeshNamesAndIndex = std::vector<std::pair<std::string,int>>();
	std::vector<std::pair<size_t, size_t>> meshStartsSizes = std::vector<std::pair<size_t, size_t>>();
	//If instancing is requested, handle first and seperately
	if (useInstancing) {
		std::vector<bool> encounteredMesh = std::vector<bool>(meshes.size());
		for (int i = 0; i < encounteredMesh.size(); i++) encounteredMesh[i] = false;
		for (int root : roots) {
			navigateSceneGraphMeshes(encounteredMesh,root);
		}
		for (size_t i = 0; i < meshes.size(); i++) {
			if (meshes[i].instanceMesh) {
				instancedMeshNamesAndIndex.push_back(std::make_pair(meshes[i].name,i));
			}
		}
		list.instancedIndexPools = std::vector<std::vector<uint32_t>>(instancedMeshNamesAndIndex.size());
		for (size_t ind = 0; ind < instancedMeshNamesAndIndex.size(); ind++) {
			instancedToPool[instancedMeshNamesAndIndex[ind].first] = ind;

			Mesh mesh = meshes[instancedMeshNamesAndIndex[ind].second];
			std::vector<SceneVertex> meshSceneVertices(vertexPool.begin() + mesh.vertexOffset, vertexPool.begin() + mesh.vertexOffset + mesh.count);
			std::vector<Vertex> meshVertices(meshSceneVertices.size());
			for (size_t vert = 0; vert < meshSceneVertices.size(); vert++) {
				meshVertices[vert].color = meshSceneVertices[vert].color;
				meshVertices[vert].pos = meshSceneVertices[vert].pos;
				meshVertices[vert].normal = meshSceneVertices[vert].normal;
			}
			meshStartsSizes.push_back(std::make_pair(list.instancedVertexPool.size(), meshVertices.size()));
			list.instancedVertexPool.reserve(list.instancedIndexPools.size() + meshVertices.size());
			list.instancedVertexPool.insert(list.instancedVertexPool.end(), meshVertices.begin(), meshVertices.end());
			if (mesh.indicies.has_value()){
				list.instancedIndexPools[ind] = *mesh.indicies;
			}
			else {
				for (int index = 0; index < meshVertices.size(); index++) {
					list.instancedIndexPools[ind].push_back(index);
				}
			}
			if (mesh.material.has_value()) {
				list.instancedMaterials.push_back(processMaterial(materials[*mesh.material]));
			}
			else {
				std::optional<Material> unprocessedMat;
				if (mesh.material.has_value()) {
					unprocessedMat = materials[*mesh.material];
				}
				list.instancedMaterials.push_back(processMaterial(unprocessedMat));
			}
		}
		uint32_t previousLast = 0;
		uint32_t nextLast = 0;
		for (size_t pool = 0; pool < list.instancedIndexPools.size(); pool++) {
			for (size_t index = 0; index < list.instancedIndexPools[pool].size(); index++) {
				list.instancedIndexPools[pool][index] += previousLast;
				if (list.instancedIndexPools[pool][index] > nextLast) {
					nextLast = list.instancedIndexPools[pool][index];
				}
			}
			previousLast = nextLast + 1;
		}
		
		//Create bounding spheres
		for (size_t pool = 0; pool < meshStartsSizes.size(); pool++) {
			size_t start = meshStartsSizes[pool].first;
			size_t end = meshStartsSizes[pool].first + meshStartsSizes[pool].second;
			std::vector<Vertex> vertices = std::vector<Vertex>(list.instancedVertexPool.begin() + start
				, list.instancedVertexPool.begin() + end);
			list.instancedBoundingSpheres.push_back(DrawNode::produceBoundingSphere(vertices));
		}
	}
	//Get intermediate parsed data annd do final parsing
	DrawListIntermediate intermediate = navigateSceneGraphInt(instancedMeshNamesAndIndex.size());
	list.cameras = intermediate.cameras;
	list.cameraDrivers = intermediate.cameraDrivers;
	list.nodeDrivers = intermediate.nodeDrivers;
	list.vertexPool = intermediate.vertexPool;
	list.textureMaps = intermediate.textureMaps;
	list.cubeMaps = intermediate.cubeMaps;
	list.environmentMap = intermediate.environmentMap;
	list.worldToLights = std::vector<mat44<float>>();
	list.lights = toDrawLights(intermediate.lights, list.worldToLights);
	std::optional<mat44<float>> worldToEnvironment = intermediate.worldToEnvironment;
	
	//Data that needs to be further parsed
	std::vector<DrawNode> drawInter = intermediate.drawPool;
	std::vector<mat44<float>> traInter = intermediate.transforms;
	std::vector<mat44<float>> traNormInter = intermediate.normalTransforms;
	std::vector<std::vector<mat44<float>>> instTraInter = intermediate.instancedTransforms;
	std::vector<std::vector<mat44<float>>> instTraNormInter = intermediate.instancedNormalTransforms;
	std::vector<uint32_t> indInter = intermediate.indexPool;
	list.transformPools = std::vector<std::vector<mat44<float>>>();
	list.instancedTransformPools = std::vector<std::vector<mat44<float>>>();
	list.normalTransformPools = std::vector<std::vector<mat44<float>>>();
	list.instancedNormalTransformPools = std::vector<std::vector<mat44<float>>>();
	list.indexPools = std::vector<std::vector<uint32_t>>();
	list.drawPools = std::vector<std::vector<DrawNode>>();
	

	//Produce pools and update indexes appropriately
	int begin = 0; int end = 1;
	for (; end <= intermediate.drawPool.size(); end++) {
		if (end - begin == poolSize || (end == intermediate.drawPool.size())) {
			int poolSize = end - begin;
			//Produce pools
			//Transform and draw pools
			std::vector<DrawNode> drawPool; drawPool.reserve(poolSize);
			std::vector<mat44<float>> transforms; transforms.reserve(poolSize);
			std::vector<mat44<float>> normalTransforms; normalTransforms.reserve(poolSize);
			drawPool.insert(drawPool.begin(), drawInter.begin() + begin, drawInter.begin() + end);
			list.drawPools.push_back(drawPool);
			transforms.insert(transforms.begin(), traInter.begin() + begin, traInter.begin() + end);
			normalTransforms.insert(normalTransforms.begin(), traNormInter.begin() + begin, traNormInter.begin() + end);
			list.transformPools.push_back(transforms);
			list.normalTransformPools.push_back(normalTransforms);
			int indexStart = drawPool.begin()->indexStart;
			int indexEnd = drawPool[poolSize - 1].indexStart + drawPool[poolSize - 1].indexCount;
			std::vector<uint32_t> indexPool; indexPool.reserve(indexEnd - indexStart);
			indexPool.insert(
				indexPool.begin(), 
				indInter.begin() + indexStart, 
				indInter.begin() + indexEnd);
			list.indexPools.push_back(indexPool);
			begin = end;
		}
	}
	//Do the same for the instance transform pools
	for (size_t pool = 0; pool < instTraInter.size(); pool++) {
		for (size_t index = 0; index < instTraInter[pool].size(); index += poolSize) {
			size_t size = index + poolSize > instTraInter[pool].size() ? instTraInter[pool].size() - index : poolSize;
			std::vector<mat44<float>> subPool = std::vector <mat44<float>>(instTraInter[pool].begin() + index, instTraInter[pool].begin() + index + size);
			std::vector<mat44<float>> subPoolNormal = std::vector <mat44<float>>(instTraNormInter[pool].begin() + index, instTraNormInter[pool].begin() + index + size);
			list.instancedTransformPools.push_back(subPool);
			list.instancedTransformIndexPools.push_back(pool);
			list.instancedNormalTransformPools.push_back(subPoolNormal);
		}
	}

	//Update index counts in each drawPool
	for (int i = 0; i < list.drawPools.size(); i++) {
		int indexStart = list.drawPools[i][0].indexStart;
		for (size_t j = 0; j < list.drawPools[i].size(); j++) {
			list.drawPools[i][j].indexStart -= indexStart;
		}
	}
	int lastNode = -1;
	for (size_t i = 0; i < list.indexPools.size(); i++) {
		auto& pool = list.indexPools[i];
		for (size_t j = 0; j < pool.size(); j++) {
			auto index = pool[j];
			Vertex vert = list.vertexPool[index];
			if (vert.node != lastNode) {
				assert(vert.node == lastNode + 1);
				lastNode = vert.node;
			}
		}
	}
	//Update vertex node ids
	for (int pool = 0; pool < list.indexPools.size(); pool++) {
		//Since vertices and indices are stored in order traveresed,
		//This can be safely assumed. IF this rule is broken, this could
		//casue errors, but its easier to just sort the data when being
		//proccessed then trying to do it here
		int nodeStart = list.vertexPool[list.indexPools[pool][0]].node;
		std::vector<uint32_t> indicesToOffset = std::vector<uint32_t>();
		for (int j = 0; j < list.indexPools[pool].size(); j++) {
			int curNode = list.indexPools[pool][j];
			indicesToOffset.push_back(curNode);
		}
		std::sort(indicesToOffset.begin(), indicesToOffset.end());
		uint32_t last = -1;
		for (int k = 0; k < indicesToOffset.size(); k++) {
			if (indicesToOffset[k] != last) {
				last = indicesToOffset[k];
				list.vertexPool[last].node -= nodeStart;
			}
		}
	}
	//Create material and material pools
	list.materialPools = std::vector<std::vector<DrawMaterial>>(list.drawPools.size());
	for (size_t pool = 0; pool < list.materialPools.size(); pool++) {
		list.materialPools[pool] = std::vector<DrawMaterial>(list.drawPools[pool].size());
		for (size_t nodeInd = 0; nodeInd < list.materialPools[pool].size(); nodeInd++) {
			list.materialPools[pool][nodeInd] = processMaterial(list.drawPools[pool][nodeInd].material);
		}
	}
	if (worldToEnvironment.has_value()) {
		//Find object to environment transforms
		list.environmentTransformPools = std::vector<std::vector<mat44<float>>>(list.transformPools.size());
		list.instancedEnvironmentTransformPools = std::vector<std::vector<mat44<float>>>(list.instancedTransformPools.size());
		//standard case
		for (size_t pool = 0; pool < list.transformPools.size(); pool++) {
			for (size_t ind = 0; ind < list.transformPools[pool].size(); ind++) {
				list.environmentTransformPools[pool].push_back(*worldToEnvironment * list.transformPools[pool][ind]);
			}
		}
		//instanced case
		for (size_t pool = 0; pool < list.instancedTransformPools.size(); pool++) {
			for (size_t ind = 0; ind < list.instancedTransformPools[pool].size(); ind++) {
				list.instancedEnvironmentTransformPools[pool].push_back(*worldToEnvironment * list.instancedTransformPools[pool][ind]);
			}
		}
		//Handle normal transforms
		for (size_t pool = 0; pool < list.transformPools.size(); pool++) {
			for (size_t ind = 0; ind < list.transformPools[pool].size(); ind++) {
				list.normalTransformPools[pool][ind] = (*environmentToWorld).transpose() * list.normalTransformPools[pool][ind];
			}
		}
		//instanced case
		for (size_t pool = 0; pool < list.instancedTransformPools.size(); pool++) {
			for (size_t ind = 0; ind < list.instancedTransformPools[pool].size(); ind++) {
				list.instancedNormalTransformPools[pool][ind] = (*environmentToWorld).transpose() * list.instancedNormalTransformPools[pool][ind];
			}
		}
	}


	//Handle case when a scene graph doesnt supply a proper camera
	if (list.cameras.size() == 0) {
		mat44<float> persp = mat44<float>::perspective(45.0, 16.0 / 9.0, 0.01, 1000);
		DrawCamera defaultCamera;
		defaultCamera.name = "default";
		defaultCamera.perspective = persp;
		defaultCamera.transform = mat44<float>(1);
		defaultCamera.perspectiveInfo.aspect = 16.0 / 9.0;
		defaultCamera.perspectiveInfo.vfov = 45.0;
		defaultCamera.perspectiveInfo.nearP = 0.01;
		defaultCamera.perspectiveInfo.farP = 1000;
		defaultCamera.forAnimate.parent = mat44<float>(1);
		defaultCamera.forAnimate.rotation = quaternion<float>::angleAxis(0, float_3(0, 0, 1));
		defaultCamera.forAnimate.scale = float_3(1,1,1);
		defaultCamera.forAnimate.translate = float_3(0,0,0);
		list.cameras.push_back(defaultCamera);
	}
	//Make a perspective world to light vector for shadow mapping
	list.worldToLightsPersp = std::vector<mat44<float>>();
	for (int i = 0; i < list.worldToLights.size(); i++) {
		mat44<float> transform = list.worldToLights[i];
		DrawLight light = list.lights[i];
		mat44<float> persp = mat44<float>::perspective(light.fov, 1, 0.01, 1000);
		list.worldToLightsPersp.push_back(persp*transform);
	}

	if (list.lights.size() == 0) {
		DrawLight light = {};
		light.type = LIGHT_NONE;
		list.lights.push_back(light);
	}
	if (list.materialPools.size() == 0) {
		DrawMaterial mat = {};
		mat.type = MAT_NONE;
		list.materialPools.resize(list.transformPools.size());
		for (int i = 0; i < list.transformPools.size(); i++) {
			for (int j = 0; j < i; j++) {
				list.materialPools[i].push_back(mat);
			}
		}
	}
	if (list.instancedMaterials.size() == 0) {
		DrawMaterial mat = {};
		mat.type = MAT_NONE;
		list.materialPools.resize(list.transformPools.size());
		for (int i = 0; i < list.transformPools.size(); i++) {
			list.instancedMaterials.push_back(mat);
		}
	}
	if (list.worldToLights.size() == 0) {
		list.worldToLights.push_back(mat44<float>(1));
		list.worldToLightsPersp.push_back(mat44<float>(1));
	}

	std::chrono::high_resolution_clock::time_point last = std::chrono::high_resolution_clock::now();
	if(verbose) std::cout << "MEASURE scene graph navigate: " << (float)
		std::chrono::duration_cast<std::chrono::milliseconds>(
			last - start).count() << "ms" << std::endl;
	return list;
}
