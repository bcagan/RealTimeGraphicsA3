#pragma once
#include "SceneGraph.h"
#include "FileHelp.h"
#include "Events.h"


std::string parseName(std::string nameString) {
	if (nameString.size() <= 8) {
		throw std::runtime_error("ERROR: Unable to parse name in Parser.");
	}
	int startBlock = 8; int endBlock = 2;
	return nameString.substr(startBlock, nameString.size() - startBlock - endBlock);
}

enum PartialMaterialType { PART_VEC, PART_FLO, PART_TEX };
struct PartialMaterialData {

	std::pair<Texture, std::string> texture;
	float_3 vec;
	float value;
	PartialMaterialType type;
};

//Class to parse different data files
class Parser {
public:
	//Parse an s72 json file
	SceneGraph parseJson(std::string fileName, bool verbose = false);
	//Parse a headless events file
	HeadlessEvents parseEvents(std::string fileName);

	//Helper functions
	static std::vector<int> parseArrayStringI(std::string arrayString);
	static std::vector<float> parseArrayStringF(std::string arrayString);


private:
	//Given an array of strings, parse into an array of array of strings
	//consisting of all structures surrounded by { .... } on the highest level
	std::vector<std::vector<std::string>> parseToObjects(std::vector<char> rawString);
	//Parse all individual lines itno seperate strings from a raw data stream.
	//Optional boolean to pre-handle certain aspects of json object handling
	std::vector<std::string> parseIntoStrings(std::vector<char> rawStringData, bool forObjects = true);

	//Helper functions
	std::vector<uint32_t> parseIndices(std::string indiciesString);
	std::vector<SceneVertex> parseAttributes(std::vector<std::string> attributeStrings);
	std::vector<float_4> parseAttribute4(std::string attributeString, bool bit_32, int stride);
	std::vector<float_2> parseAttribute2(std::string attributeString, int stride);
	std::map<std::string, int> nameToTexture;
	int nameToTextureId(std::string name, Texture tex, std::vector<Texture>* textures);
	std::map<std::string, int> nameToCube;
	int nameToCubeId(std::string name, Texture tex, std::vector<Texture>* textures);
	PBRInt parsePBR(
		std::string albedoString,
		std::string roughnessString,
		std::string specularString);
	PartialMaterialData parseMaterialData(std::string objectStringtextures, bool parseAsCube = false);

	//Specific object type parsing functions
	Camera parseCamera(std::vector<std::string> jsonObject);
	Light parseLight(std::vector<std::string> jsonObject);
	GraphNode parseNode(std::vector<std::string> jsonObject);
	SceneDriver parseDriver(std::vector<std::string> jsonObject);
	MaterialInt parseMaterial(std::vector<std::string> jsonObject);
};

PartialMaterialData Parser::parseMaterialData(std::string objectString, bool parseAsCube) {
	PartialMaterialData parsedData;
	if (objectString.at(0) == '[') {
		parsedData.type = PART_VEC;
		std::vector<float> stringVec = Parser::parseArrayStringF(objectString);
		float_3 dataVec;
		dataVec.x = stringVec[0];
		dataVec.y = stringVec[1];
		dataVec.z = stringVec[2];
		parsedData.vec = dataVec;
	}
	else if (objectString.size() > 3 && objectString.at(3) == 's') {
		parsedData.type = PART_TEX;
		std::string texName = objectString.substr(9, objectString.size() - 10);
		Texture parsedTex = Texture::parseTexture(texName, parseAsCube);
		std::pair<Texture, std::string> texData = std::make_pair(parsedTex, texName);
		parsedData.texture = texData;
	}
	else {
		parsedData.type = PART_FLO;
		parsedData.value = atof(objectString.c_str());
	}
	return parsedData;
}

PBRInt Parser::parsePBR(
	std::string albedoString, 
	std::string roughnessString, 
	std::string specularString) {
	PBRInt pbr;
	pbr.useValueAlbedo = true;
	pbr.useValueRoughness = true;
	pbr.useValueSpecular = true;
	PartialMaterialData albedoData = parseMaterialData(albedoString);
	PartialMaterialData roughnessData = parseMaterialData(roughnessString);
	PartialMaterialData specularData = parseMaterialData(specularString);
	if (albedoData.type == PART_TEX) {
		pbr.useValueAlbedo = false;
		pbr.albedo.texture = albedoData.texture;
	}
	else {
		pbr.albedo.value = albedoData.vec;
	}
	if (specularData.type == PART_TEX) {
		pbr.useValueSpecular = false;
		pbr.specular.texture = specularData.texture;
	}
	else {
		pbr.specular.value = specularData.value;
	}
	if (roughnessData.type == PART_TEX) {
		pbr.useValueRoughness = false;
		pbr.roughness.texture = roughnessData.texture;
	}
	else {
		pbr.roughness.value = roughnessData.value;
	}
	return pbr;
}

MaterialInt Parser::parseMaterial(std::vector<std::string> jsonObject) {
	MaterialInt parsedMaterial;
	for (size_t stringInd = 0; stringInd < jsonObject.size(); stringInd++) {
		std::string objectString = jsonObject[stringInd];
		std::string compString = jsonObject[stringInd];
		if (objectString.at(objectString.size() - 1) == ',') {
			objectString = objectString.substr(0, objectString.size() - 1);
			compString = compString.substr(0, compString.size() - 1);
		}
		compString = compString.substr(1, 2);
		size_t objLen = objectString.size();
		if (objLen > 9 && !compString.compare("na")) {
			parsedMaterial.name = objectString.substr(8, objLen - 1);
		}
		else if (objLen > 21 && !compString.compare("no")) {
			std::string texName = objectString.substr(21, objLen - 23);
			parsedMaterial.normalMap = std::make_pair(Texture::parseTexture(texName,false), texName);
		}
		else if (objLen > 27 && !compString.compare("di")) {
			std::string texName = objectString.substr((27, objLen - 30));
			parsedMaterial.displacementMap = std::make_pair(Texture::parseTexture(texName,false), texName);
		}
		else if (objLen > 3 && !compString.compare("pb")) {
			parsedMaterial.type = MAT_PBR;
			std::string albedoString = jsonObject[stringInd + 1];
			std::string roughnessString = jsonObject[stringInd + 2];
			std::string specularString = jsonObject[stringInd + 3];
			if (albedoString.at(10) == '{')albedoString = albedoString.substr(13, albedoString.size() - 14);
			else albedoString = albedoString.substr(10, albedoString.size() - 11);
			if (roughnessString.at(13) == '{')roughnessString = roughnessString.substr(16, roughnessString.size() - 17);
			else roughnessString = roughnessString.substr(13, roughnessString.size() - 14);
			if (specularString.at(13) == '{')specularString = specularString.substr(16, specularString.size() - 17);
			else specularString = specularString.substr(13, specularString.size() - 14);;
			parsedMaterial.data.pbr.set(parsePBR(albedoString, roughnessString, specularString));
			return parsedMaterial;
		}
		else if (objLen > 3 && !compString.compare("la")) {
			parsedMaterial.type = MAT_LAM;
			std::string lambertianString =
				jsonObject[stringInd + 1];
			lambertianString = 
				lambertianString.substr(10, lambertianString.size() - 11);
			PartialMaterialData partialData = parseMaterialData(lambertianString, false);
			if (partialData.type == PART_TEX) {
				parsedMaterial.data.lambertian.useValue = false;
				parsedMaterial.data.lambertian.texture = partialData.texture;
			}
			else {
				parsedMaterial.data.lambertian.useValue = true;
				parsedMaterial.data.lambertian.value = partialData.vec;
			}
		}
		else if (objLen > 3 && !compString.compare("mi")) {
			parsedMaterial.type = MAT_MIR;
			parsedMaterial.data.mirror = true;
			return parsedMaterial;
		}
		else if (objLen > 3 && !compString.compare("en")) {
			parsedMaterial.type = MAT_ENV;
			parsedMaterial.data.environment = true;
			return parsedMaterial;
		}
		else if (objLen > 3 && !compString.compare("si")) {
			parsedMaterial.type = MAT_SIM;
			parsedMaterial.data.simple = true;
			return parsedMaterial;
		}
	}
}

SceneDriver Parser::parseDriver(std::vector<std::string> jsonObject) {
	SceneDriver driver;
	for (std::string objectString : jsonObject) {
		if (objectString.at(objectString.size() - 1) == ',') {
			objectString = objectString.substr(0, objectString.size() - 1);
		}
		size_t objLen = objectString.size();
		if (objLen > 9 && !objectString.substr(1, 2).compare("na")) {
			driver.name = objectString.substr(8, objLen - 1);
		}
		else if (objLen > 7 && !objectString.substr(1, 2).compare("no")) {
			driver.index = atoi(objectString.substr(7, objLen - 7).c_str());
		}
		else if (objLen > 11 && !objectString.substr(1, 2).compare("ch")) {
			std::string channelString = objectString.substr(11, objLen - 12);
			if (!channelString.compare("translation")) {
				driver.channel = CH_TRANSLATE;
			}
			else if (!channelString.compare("scale")) {
				driver.channel = CH_SCALE;
			}
			else if (!channelString.compare("rotation")) {
				driver.channel = CH_ROTATE;
			}
			else {
				throw std::runtime_error("ERROR: Invalid channel found when parsing json file in Parser.");
			}
		}
		else if (objLen > 8 && !objectString.substr(1, 2).compare("ti")) {
			driver.times = parseArrayStringF(objectString.substr(8, objLen - 8));
		}
		else if (objLen > 9 && !objectString.substr(1, 2).compare("va")) {
			driver.values = parseArrayStringF(objectString.substr(9, objLen - 9));
		}
		else if (objLen > 17 && !objectString.substr(1, 2).compare("in")) {
			std::string interpString = objectString.substr(17, objLen - 18);
			if (!interpString.compare("LINEAR")) {
				driver.interpolation = LINEAR;
			}
			else if (!interpString.compare("STEP")) {
				driver.interpolation = STEP;
			}
			else if (!interpString.compare("SLERP")) {
				driver.interpolation = SLERP;
			}
			else {
				throw std::runtime_error("ERROR: Invalid interpolation found when parsing json file in Parser.");
			}
		}
	}
	return driver;
}

GraphNode Parser::parseNode(std::vector<std::string> jsonObject) {
	std::string nameString = jsonObject[1];
	nameString = parseName(nameString);
	GraphNode parsedNode;
	parsedNode.name = nameString;
	parsedNode.translate = vec3<float>(0, 0, 0);
	parsedNode.scale = vec3<float>(1, 1, 1);
	parsedNode.rotation = quaternion<float>::angleAxis(0, float_3(0, 0, 1));
	//Noting that the propertie strings are varying lenght but,
	//have a unique starting 2 chars, case on that
	for (size_t strInd = 2; strInd < jsonObject.size(); strInd++) {
		std::string objectString = jsonObject[strInd];
		if (objectString.at(objectString.size() - 1) == ',') {
			objectString = objectString.substr(0, objectString.size() - 1);
		}
		size_t objLen = objectString.size();
		//Translation
		if (!objectString.substr(1, 2).compare("tr")) {
			std::string arrayString = objectString.substr(14, objLen - 14);
			std::vector<float> trArr = parseArrayStringF(arrayString);
			if (trArr.size() < 3) {
				throw std::runtime_error("ERROR: Invalid translation array found in node when parsing json file in Parser.");
			}
			parsedNode.translate = float_3(trArr[0], trArr[1], trArr[2]);
		}
		//Rotation
		else if(!objectString.substr(1, 2).compare("ro")) {
			std::string arrayString = objectString.substr(11, objLen - 11);
			std::vector<float> rotationArr = parseArrayStringF(arrayString);
			if (rotationArr.size() < 4) {
				throw std::runtime_error("ERROR: Invalid rotation array found in node when parsing json file in Parser.");
			}
			parsedNode.rotation.setAngle(rotationArr[3]);
			parsedNode.rotation.setAxis(
				float_3(rotationArr[0], rotationArr[1], rotationArr[2])
			);
		}
		//Scale
		else if(!objectString.substr(1, 2).compare("sc")) {
			std::string arrayString = objectString.substr(8, objLen - 8);
			std::vector<float> scaleArr = parseArrayStringF(arrayString);
			if (scaleArr.size() < 3) {
				throw std::runtime_error("ERROR: Invalid scale array found in node when parsing json file in Parser.");
			}
			parsedNode.scale = float_3(scaleArr[0], scaleArr[1], scaleArr[2]);
		}
		//Children, optional
		else if (!objectString.substr(1, 2).compare("ch")) {
			std::string arrayString = objectString.substr(11, objLen - 11);
			parsedNode.children = parseArrayStringI(arrayString);
		}
		//Camera, optional
		else if (!objectString.substr(1, 2).compare("ca")) {
			parsedNode.camera = atoi(objectString.substr(9, objLen - 9).c_str());
		}
		//Light, optional
		else if (!objectString.substr(1, 2).compare("li")) {
			parsedNode.light = atoi(objectString.substr(8, objLen - 8).c_str());
		}
		//Environment, optional
		else if (!objectString.substr(1, 2).compare("en")) {
			parsedNode.hasEnvironment = true;
		}
		//Mesh
		else if(!objectString.substr(1, 2).compare("me")) {
			std::string meshStr = objectString.substr(7, objLen - 7);
			int meshInt = atoi(meshStr.c_str());
			parsedNode.mesh = atoi(meshStr.c_str());
		}
	}
	return parsedNode;
}

Camera Parser::parseCamera(std::vector<std::string> jsonObject) {
	std::string nameString = jsonObject[1];
	nameString = parseName(nameString);
	Camera parsedCamera;
	parsedCamera.name = nameString;

	std::string aspectString = jsonObject[3];
	std::string vfovString = jsonObject[4];
	std::string nearString = jsonObject[5];
	std::string farString = jsonObject[6];

	float aspect = atof(aspectString.substr(9, aspectString.size() - 9).c_str());
	float vfov = atof(vfovString.substr(7, vfovString.size() - 7).c_str());
	float nearVal = atof(nearString.substr(7, nearString.size() - 7).c_str());
	float farVal = atof(farString.substr(6, farString.size() - 6).c_str());

	parsedCamera.perspective.aspect = aspect;
	parsedCamera.perspective.vfov = vfov;
	parsedCamera.perspective.nearP = nearVal;
	parsedCamera.perspective.farP = farVal;
	return parsedCamera;
}

Light Parser::parseLight(std::vector<std::string> jsonObject) {
	std::string nameString = jsonObject[1];
	nameString = parseName(nameString);
	Light parsedLight;
	parsedLight.shadowRes = 0;
	parsedLight.type = LIGHT_NONE;

	for (size_t i = 1; i < jsonObject.size(); i++) {
		std::string jsonString = jsonObject[i];
		size_t strSize = jsonString.size();
		if (strSize < 4) {
			throw std::runtime_error("ERROR: Invalid string found in light object in Parser.");
		}
		char json1 = jsonString.at(1); 
		char json2 = jsonString.at(2);
		char json3 = jsonString.at(3);
		if (json1 == 't' && json2 == 'i') {
			std::vector<float> tintVec = parseArrayStringF(jsonString.substr(7, strSize - 8));
			if (tintVec.size() != 3) {
				throw std::runtime_error("ERROR: Incorrect format for light tint found in Parser.");
			}
			else{
				parsedLight.tintR = tintVec[0];
				parsedLight.tintG = tintVec[1];
				parsedLight.tintB = tintVec[2];
			}
		}
		else if (json1 == 's' && json2 == 'u') {
			parsedLight.type = LIGHT_SUN;
			std::string angleStr = jsonObject[i + 1];
			std::string strengthStr = jsonObject[i + 2];
			size_t angleSize = angleStr.size();
			size_t streSize = strengthStr.size();
			parsedLight.angle = atof(angleStr.substr(8, angleSize - 8).c_str());
			parsedLight.strength = atof(strengthStr.substr(11, streSize - 11).c_str());
			i += 3;
		}
		else if (json1 == 's' && json2 == 'p' && json3 == 'h') {
			parsedLight.type = LIGHT_SPHERE;
			std::string radiusStr = jsonObject[i + 1];
			std::string powerStr = jsonObject[i + 2];
			size_t radiusSize = radiusStr.size();
			size_t powerSize = powerStr.size();
			parsedLight.radius = atof(radiusStr.substr(8, radiusSize - 9).c_str());
			parsedLight.power = atof(powerStr.substr(8, powerSize - 8).c_str());
			if (jsonObject[i + 3].at(0) == '}') {
				i += 3;
			}
			else {
				std::string limitStr = jsonObject[i + 3];
				size_t limSize = limitStr.size();
				parsedLight.limit = atof(limitStr.substr(8, limSize - 8).c_str());
				i += 4;
			}
		}
		else if (json1 == 's' && json2 == 'p' && json3 == 'o') {
			parsedLight.type = LIGHT_SPOT;
			bool hasLimit = jsonObject[i + 5].at(0) != '}';
			std::string radiusStr = jsonObject[i + 1];
			std::string powerStr = jsonObject[i + 2];
			std::string fovStr = jsonObject[i + 3];
			std::string blendStr = jsonObject[i + 4];
			size_t radiusSize = radiusStr.size();
			size_t powerSize = powerStr.size();
			size_t fovSize = fovStr.size();
			size_t blendize = blendStr.size();
			parsedLight.radius = atof(radiusStr.substr(8, radiusSize - 9).c_str());
			parsedLight.power = atof(powerStr.substr(8, powerSize - 8).c_str());
			parsedLight.fov = atof(fovStr.substr(6, fovSize - 6).c_str());
			parsedLight.blend = atof(blendStr.substr(8, blendize - 8).c_str());
			if (hasLimit) {
				std::string limitStr = jsonObject[i + 5];
				size_t limSize = limitStr.size();
				parsedLight.limit = atof(limitStr.substr(8, limSize - 8).c_str());
				i += 6;
			}
			else {

				i += 5;
			}
		}
		else if (json1 == 's' && json2 == 'h') {
			parsedLight.shadowRes = atoi(jsonString.substr(9, strSize - 9).c_str());
		}
	}

	return parsedLight;
}

//Parses a string consisting of an array of integers denoted by [...] into 
//a corresponding integer vector 
std::vector<int> Parser::parseArrayStringI(std::string arrayString) {
	//Assert inputted string is an array
	if (arrayString.at(0) != '[' || arrayString.at(arrayString.size() - 1) != ']') {
		throw std::runtime_error("ERROR: Unable to int parse array in json file in Parser.");
	}
	std::vector<int> parsedArray;
	int begin = 1;
	for (int end = begin + 1; end < arrayString.size(); end++) {
		if (arrayString.at(end) == ',' || arrayString.at(end) == ']') {
			parsedArray.push_back(atoi(arrayString.substr(begin, end - begin).c_str()));
			begin = end + 1;
		}
	}
	return parsedArray;
}

//Parses a string consisting of an array of floats denoted by [...] into 
//a corresponding float vector
std::vector<float> Parser::parseArrayStringF(std::string arrayString) {
	//Assert inputted string is an array
	if (arrayString.at(0) != '[' || arrayString.at(arrayString.size() - 1) != ']') {
		throw std::runtime_error("ERROR: Unable to parse float array in json file in Parser.");
	}
	std::vector<float> parsedArray;
	int begin = 1;
	for (int end = begin + 1; end < arrayString.size(); end++) {
		if (arrayString.at(end) == ',' || arrayString.at(end) == ']') {
			parsedArray.push_back(atof(arrayString.substr(begin, end - begin).c_str()));
			begin = end + 1;
		}
	}
	return parsedArray;
}

//Given a string and a start and ending token, return the specific substiring
std::string findSegment(std::string str, std::string startKey, std::string endKey) {
	size_t start = 0; size_t size = 0;
	std::string segment;

	while (start + startKey.size() < str.size()) {
		if (str.substr(start, startKey.size()).compare(startKey) == 0) break;
		start++;
	}
	start += startKey.size();
	while (start + size + endKey.size() < str.size()) {
		if (str.substr(start + size,endKey.size()).compare(endKey) == 0) break;
		size++;
	}
	if (start + size + endKey.size() >= str.size()) {
		throw std::runtime_error("ERROR: Cannot find segment given inputs in Parser.");
	}
	return str.substr(start, size);
}

//Parses indicies attributes, including source file, from a string, then loads
//the corresponding data
std::vector<uint32_t> Parser::parseIndices(std::string indiciesString) {
	std::string srcString = findSegment(indiciesString, "src\":\"", "\",");
	std::string offsetString = findSegment(indiciesString, "offset\":", ",");
	//format = UINT32;
	size_t offset = atoi(offsetString.c_str());
	std::vector<char> rawData = readFile(srcString);
	std::vector<uint32_t> data;
	for (size_t ind = offset; ind < rawData.size(); ind += 4) {
		data.push_back(*(uint32_t*)(rawData.data() + ind));
	}
	return data;
}


std::vector<SceneVertex> Parser::parseAttributes(std::vector<std::string> attributeStrings) {


	int stride = 0;
	std::optional<std::string> positionString;
	std::optional<std::string> normalString;
	std::optional<std::string> tangentString;
	std::optional<std::string> texcoordsString;
	std::optional<std::string> colorString;
	for (std::string attributeString : attributeStrings) {
		if (attributeString.substr(1, 2) == "PO") {
			stride += 12;
			positionString = attributeString;
		}
		else if (attributeString.substr(1, 2) == "NO") {
			stride += 12;
			normalString = attributeString;
		}
		else if (attributeString.substr(1, 2) == "TA") {
			stride += 16;
			tangentString = attributeString;
		}
		else if (attributeString.substr(1, 2) == "TE") {
			stride += 8;
			texcoordsString = attributeString;
		}
		else if (attributeString.substr(1, 2) == "CO") {
			stride += 4;
			colorString = attributeString;
		}
	}
	std::vector<float_4> positions;
	std::vector<float_4> normals;
	std::vector<float_4> tangents;
	std::vector<float_2> texcoords;
	std::vector<float_4> colors;
	if (positionString.has_value()) {
		positions = parseAttribute4(*positionString, true, stride);
	}
	if (normalString.has_value()){
		normals = parseAttribute4(*normalString, true, stride);
	}
	if (tangentString.has_value()) {
		tangents = parseAttribute4(*tangentString, true, stride);
	}
	if (texcoordsString.has_value()) {
		texcoords = parseAttribute2(*texcoordsString, stride);
	}
	if (colorString.has_value()) {
		colors = parseAttribute4(*colorString, false, stride);
	}
	//Create vertices and insert into vertex pool
	std::vector<SceneVertex> vertices(positions.size());
	for (size_t vert = 0; vert < vertices.size(); vert++) {
		if (positionString.has_value()) {
			vertices[vert].pos = (float_3)positions[vert];
		}
		if (normalString.has_value()) {
			vertices[vert].normal = (float_3)normals[vert];
		}
		if (tangentString.has_value()) {
			vertices[vert].tangent = tangents[vert];
		}
		if (texcoordsString.has_value()) {
			vertices[vert].texcoord = (float_2)texcoords[vert];
		}
		if (colorString.has_value()) {
			vertices[vert].color = colors[vert];
		}
	}
	return vertices;
}

//Loads and parses attribute data into an R32G32B32A32 form
std::vector<float_4> Parser::parseAttribute4(std::string attributeString, bool bit_32, int stride) {

	//Find source
	std::string srcString = std::string("Scenes/").append(findSegment(attributeString, "src\":\"", "\","));
	std::string offsetString = findSegment(attributeString, "offset\":", ",");
	size_t offset = atoi(offsetString.c_str());
	std::vector<char> rawData = readFile(srcString);
	std::vector<float_4> data;
	for (size_t ind = offset; ind < rawData.size(); ind += stride) {
		if (bit_32) {
			float x = (*(float*)(rawData.data() + ind));
			float y = (*(float*)(rawData.data() + ind + 4));
			float z = (*(float*)(rawData.data() + ind + 8));
			data.push_back(float_4(x, y, z, 0));
		}
		else {

			unsigned char xC = rawData[ind];
			unsigned char yC = rawData[ind + 1];
			unsigned char zC = rawData[ind + 2];
			unsigned char wC = rawData[ind + 3];
			float x = (((float)xC) / 255.0);
			float y = (((float)yC) / 255.0);
			float z = (((float)zC) / 255.0);
			float w = (((float)wC) / 255.0);
			data.push_back(float_4(x, y, z, w));
		}
	}
	return data;
}

//Loads and parses attribute data into an R32G32 form
std::vector<float_2> Parser::parseAttribute2(std::string attributeString, int stride) {

	//Find source
	std::string srcString = std::string("Scenes/").append(findSegment(attributeString, "src\":\"", "\","));
	std::string offsetString = findSegment(attributeString, "offset\":", ",");
	size_t offset = atoi(offsetString.c_str());
	std::vector<char> rawData = readFile(srcString);
	std::vector<float_2> data;
	for (size_t ind = offset; ind < rawData.size(); ind += stride) {
		float x = (*(float*)(rawData.data() + ind));
		float y = (*(float*)(rawData.data() + ind + 4));
		data.push_back(float_2(x, y));
	}
	return data;
}


int Parser::nameToTextureId(std::string name, Texture tex, std::vector<Texture>* textures) {
	std::map<std::string, int>::iterator mapCheck = nameToTexture.find(name);
	if (mapCheck == nameToTexture.end()) {
		int texIndex = textures->size();
		nameToTexture[name] = texIndex;
		textures->push_back(tex);
		return texIndex;
	}
	return mapCheck->second;
}

int Parser::nameToCubeId(std::string name, Texture tex, std::vector<Texture>* textures) {
	std::map<std::string, int>::iterator mapCheck = nameToCube.find(name);
	if (mapCheck == nameToCube.end()) {
		int texIndex = textures->size();
		nameToCube[name] = texIndex;
		textures->push_back(tex);
		return texIndex;
	}
	return mapCheck->second;
}


std::vector<std::string> Parser::parseIntoStrings(std::vector<char> rawStringData, bool forObjects) {
	std::string rawString(rawStringData.begin(), rawStringData.end());
	std::vector<std::string> parsedStrings;
	std::vector<size_t> begins;
	std::vector<size_t> lengths;
	for (size_t c = 0; c < rawString.size(); c++) {
		if (!forObjects || ((rawString[c] == '"' && c > 0 && rawString[c - 1] == '\t') || rawString[c] == '}' || rawString[c] == '{')) {
			size_t begin = c;
			while (c < rawString.size() && rawString[c] != '\n' && (rawString[c] != '\r' || !forObjects)) c++;
			if (c >= rawString.size()) break;
			begins.push_back(begin);
			lengths.push_back(c - begin);
		}
	}
	for (size_t ind = 0; ind < begins.size(); ind++) {
		std::string parsedString = rawString.substr(begins[ind], lengths[ind]);
		parsedStrings.push_back(parsedString);
	}
	if (forObjects){
		//Format to better be parsed by parseToObjects
		parsedStrings[parsedStrings.size() - 1].append(",");
	}
	return parsedStrings;
}

std::vector<std::vector<std::string>> Parser::parseToObjects(std::vector<char> rawString) {
	std::vector<std::string> parsedStrings = parseIntoStrings(rawString);
	std::vector<std::vector<std::string>> objects;
	for (size_t line = 0; line < parsedStrings.size(); line++) {
		if (parsedStrings[line].substr(0, 1).compare("{") == 0) {
			size_t begin = line + 1;
			size_t end = begin;
			int depthCounter = 0; //Allows rudemnatary recursive objects
			for (; end < parsedStrings.size() && (depthCounter > 0 || parsedStrings[end].substr(0, 2).compare("},")); end++) {
				if (parsedStrings[end].size() > 10 && !parsedStrings[end].substr(1, 10).compare("attributes")) {
					depthCounter++;
				}
				else if (parsedStrings[end].size() > 3 && !parsedStrings[end].substr(1, 3).compare("sun")) {
					depthCounter++;
				}
				else if (parsedStrings[end].size() > 4 && !parsedStrings[end].substr(1, 4).compare("spot")) {
					depthCounter++;
				}
				else if (parsedStrings[end].size() > 6 && !parsedStrings[end].substr(1, 6).compare("sphere")) {
					depthCounter++;
				}
				if (depthCounter > 0 && parsedStrings[end].at(0) == '}') depthCounter--;
			}
			//{ - line
			//   First line - begin or end - 1
			//   .... 
			//   Last line - end - 1 or N/A
			//} - end
			if (end >= parsedStrings.size() || end -1 - begin < 0) break;
			std::vector<std::string> object = std::vector<std::string>(parsedStrings.begin() + begin, parsedStrings.begin() + end);
			objects.push_back(object);
			line = end;
		}
	}
	return objects;
}

//Given a desired .s72 file, parse the json file into a SceneGraph structure
SceneGraph Parser::parseJson(std::string fileName, bool verbose) {
	std::chrono::high_resolution_clock::time_point start =
		std::chrono::high_resolution_clock::now();
	std::vector<char> rawString = readFile(fileName);
	std::vector<std::vector<std::string>> objects = parseToObjects(rawString);
	SceneGraph parsedGraph;
	parsedGraph.graphNodes = std::vector<GraphNode>();
	int id = 0;
	std::vector<SceneDriver> tempDriverPool = std::vector<SceneDriver>();
	std::map<int, int> environmentToTexture;
	std::map<int, int> jsonIdToMeshId;
	std::map<int, int> jsonIdToMaterialId;
	std::map<int, int> jsonIdToCameraId;
	std::map<int, int> jsonIdToLightId;

	//Parsed into json objects, handle each object in order
	for (std::vector<std::string> jsonObject : objects) {
		id++;
		//Find and parse by type
		std::string typeString = jsonObject[0];
		if (typeString.size() <= 8) { 
			throw std::runtime_error("ERROR: Invalid object found when parsing json file " + fileName + ".");
		}
		typeString = typeString.substr(8, typeString.size() - 8);
		size_t c = 0;
		for (; c < typeString.size(); c++) {
			if (typeString.at(c) == '"') break;
		}
		typeString = typeString.substr(0, c);
		//Parse scene structure
		if (!typeString.compare("SCENE")) {
			std::string rootString = jsonObject[2];
			if (rootString.size() <= 8) {
				throw std::runtime_error("ERROR: Invalid object found when parsing json file " + fileName + ".");
			}
			std::string nameString = jsonObject[1];
			parsedGraph.name = parseName(nameString);
			int startBlock = 8;
			std::string arrayString = rootString.substr(startBlock, rootString.size() - startBlock);
			parsedGraph.roots = parseArrayStringI(arrayString);
		}
		//Use node helper function to parse node structure
		else if (!typeString.compare("NODE")) {
			GraphNode parsedNode = parseNode(jsonObject);
			parsedNode.index = id;
			parsedGraph.graphNodes.push_back(parsedNode);
		}
		//Directly parse mesh structure
		else if (!typeString.compare("MESH")) {
			Mesh mesh;
			bool simple = false;
			mesh.instanceMesh = false;
			std::string nameString = jsonObject[1];
			mesh.name = parseName(nameString);
			mesh.count = atoi(
				jsonObject[3].substr(8,jsonObject[3].size() - 9).c_str());

			size_t firstAttribute = 5;
			//Case on attributes
			if (jsonObject[4].substr(1, 2).compare("in") == 0) {
				firstAttribute++;
				mesh.indicies = parseIndices(jsonObject[4]);
			}
			//Parse attributes
			std::vector<std::string> attributeStrings;
			size_t attributeInd = firstAttribute;
			for (; attributeInd < jsonObject.size() && jsonObject[attributeInd].substr(1, 2).compare("ma"); attributeInd++) {
				attributeStrings.push_back(jsonObject[attributeInd]);
			}
			std::vector<SceneVertex> vertices = parseAttributes(attributeStrings);
			//Parse material
			if (attributeInd < jsonObject.size()) {
				mesh.material = atoi(jsonObject[attributeInd].substr(11, jsonObject[attributeInd].size() - 11).c_str());
			}
			mesh.vertexOffset = parsedGraph.vertexPool.size();
			parsedGraph.vertexPool.reserve(mesh.vertexOffset + mesh.count);
			parsedGraph.vertexPool.insert(
				parsedGraph.vertexPool.end(), 
				vertices.begin(), 
				vertices.end()
			);
			//indices and count
			mesh.count = vertices.size();
			jsonIdToMeshId[id] = parsedGraph.meshes.size();
			parsedGraph.meshes.push_back(mesh);
		}
		//Use camera helper function to parse camera object
		else if (!typeString.compare("CAMERA")) {
			Camera parsedCamera = parseCamera(jsonObject);
			jsonIdToCameraId[id] = parsedGraph.cameras.size();
			parsedGraph.cameras.push_back(parsedCamera);
		}
		//Use camera helper function to parse camera object
		else if (!typeString.compare("LIGHT")) {
			Light parsedLight = parseLight(jsonObject);
			jsonIdToLightId[id] = parsedGraph.lights.size();
			parsedGraph.lights.push_back(parsedLight);
		}
		//Use driver helper function to parse driver object
		else if (!typeString.compare("DRIVER")) {
			SceneDriver parsedDriver = parseDriver(jsonObject);
			tempDriverPool.push_back(parsedDriver);
		}
		else if (!typeString.compare("DATA")) {
			std::cout << "Found a data object when parsing scene graph. \n Data objects currently unsupported." << std::endl;
		}
		else if (!typeString.compare("MATERIAL")) {
			MaterialInt parsedMaterial = parseMaterial(jsonObject);
			Material material;
			material.name = parsedMaterial.name;
			material.type = parsedMaterial.type;
			if (parsedMaterial.displacementMap.has_value()) {
				material.displacementMap = nameToTextureId(
					parsedMaterial.displacementMap->second, 
					parsedMaterial.displacementMap->first, 
					&parsedGraph.textureMaps);
			}
			if (parsedMaterial.normalMap.has_value()) {
				material.normalMap = nameToTextureId(
					parsedMaterial.normalMap->second, 
					parsedMaterial.normalMap->first, 
					&parsedGraph.textureMaps);
			}
			//Manage intermediate material result;
			//Combine all material data into a single type
			switch (parsedMaterial.type) {
			case(MAT_PBR):
				material.data.useValueAlbedo =
					parsedMaterial.data.pbr.useValueAlbedo;
				material.data.useValueRoughness =
					parsedMaterial.data.pbr.useValueRoughness;
				material.data.useValueSpecular =
					parsedMaterial.data.pbr.useValueSpecular;
				if (material.data.useValueAlbedo) {
					material.data.albedo =
						parsedMaterial.data.pbr.albedo.value;
				}
				else {
					material.data.albedoTexture = nameToTextureId(
						parsedMaterial.data.pbr.albedo.texture->second,
						parsedMaterial.data.pbr.albedo.texture->first,
						&parsedGraph.textureMaps);
				}
				if (material.data.useValueRoughness) {
					material.data.roughness =
						parsedMaterial.data.pbr.roughness.value;
				}
				else {
					material.data.roughnessTexture = nameToTextureId(
						parsedMaterial.data.pbr.roughness.texture->second,
						parsedMaterial.data.pbr.roughness.texture->first,
						&parsedGraph.textureMaps);
				}
				if (material.data.useValueSpecular) {
					material.data.specular =
						parsedMaterial.data.pbr.specular.value;
				}
				else {
					material.data.specularTexture = nameToTextureId(
						parsedMaterial.data.pbr.specular.texture->second,
						parsedMaterial.data.pbr.specular.texture->first,
						&parsedGraph.textureMaps);
				}
				break;
			case(MAT_LAM):
				if (parsedMaterial.data.lambertian.useValue) {
					material.data.useValueAlbedo = true;
					material.data.albedo =
						parsedMaterial.data.lambertian.value;
				}
				else {
					material.data.useValueAlbedo = false;
					material.data.albedoTexture = nameToTextureId(
						parsedMaterial.data.lambertian.texture->second,
						parsedMaterial.data.lambertian.texture->first,
						&parsedGraph.textureMaps);
				}
				break;
			//None of the other materials use material data
			default:
				break;
			}
			jsonIdToMaterialId[id] = parsedGraph.materials.size();
			material.index = parsedGraph.materials.size();
			parsedGraph.materials.push_back(material);
		}
		else if (!typeString.compare("ENVIRONMENT")) {
			std::string fileName = findSegment(jsonObject[2], "\"src\":\"", "\"");
			parsedGraph.environmentMap = Texture::parseTexture(fileName, true);
		}
	}
	//Reformat reference ids
	//This could maybe done more efficiently by saving the mapping from json
	//object order to mesh, node, camera, etc. order
	for (SceneDriver driver : tempDriverPool) {
		for (size_t nodeIdx = 0; nodeIdx < parsedGraph.graphNodes.size(); nodeIdx++) {
			if (parsedGraph.graphNodes[nodeIdx].index == driver.index) {
				switch (driver.channel) {
				case CH_TRANSLATE:
					parsedGraph.graphNodes[nodeIdx].translateDriver = driver;
					break;
				case CH_ROTATE:
					parsedGraph.graphNodes[nodeIdx].rotateDriver = driver;
					break;
				defaut:
					parsedGraph.graphNodes[nodeIdx].scaleDriver = driver;
				}
			}
		}
	}
	for (size_t nodeIdx = 0; nodeIdx < parsedGraph.graphNodes.size(); nodeIdx++) {
		GraphNode& node = parsedGraph.graphNodes[nodeIdx];
		if (node.mesh.has_value()) {
			std::map<int, int>::iterator mapppedMesh = jsonIdToMeshId.find(*node.mesh);
			if (mapppedMesh == jsonIdToMeshId.end()) {
				throw std::runtime_error("ERROR: Unable to find mesh id for mesh in Parser.");
			}
			node.mesh = mapppedMesh->second;
		}

		if (node.camera.has_value()) {
			std::map<int, int>::iterator mappedCamera = jsonIdToCameraId.find(*node.camera);
			if (mappedCamera == jsonIdToCameraId.end()) {
				throw std::runtime_error("ERROR: Unable to find camera id for camera in Parser.");
			}
			node.camera = mappedCamera->second;
		}

		if (node.light.has_value()) {
			std::map<int, int>::iterator mappedLight = jsonIdToLightId.find(*node.light);
			if (mappedLight == jsonIdToLightId.end()) {
				throw std::runtime_error("ERROR: Unable to find light id for light in Parser.");
			}
			node.light = mappedLight->second;
		}
		std::vector<int> newChildren = std::vector<int>();
		for (size_t child = 0; child < node.children.size(); child++) {
			for (int subNodeIdx = 0; subNodeIdx < parsedGraph.graphNodes.size(); subNodeIdx++) {
				if (parsedGraph.graphNodes[subNodeIdx].index == node.children[child]) {
					newChildren.push_back(subNodeIdx);
					continue;
				}
			}
		}
		parsedGraph.graphNodes[nodeIdx].children = newChildren;
	}
	for (size_t rootIdx = 0; rootIdx < parsedGraph.roots.size(); rootIdx++) {
		for (size_t nodeIdx = 0; nodeIdx < parsedGraph.graphNodes.size(); nodeIdx++) {
			if (parsedGraph.roots[rootIdx] == parsedGraph.graphNodes[nodeIdx].index) {
				parsedGraph.roots[rootIdx] = nodeIdx;
			}
		}
	}
	for (size_t nodeIdx = 0; nodeIdx < parsedGraph.graphNodes.size(); nodeIdx++) {
		parsedGraph.graphNodes[nodeIdx].index = nodeIdx;
	}
	for (size_t cameraIdx = 0; cameraIdx < parsedGraph.cameras.size(); cameraIdx++) {
		parsedGraph.cameras[cameraIdx].index = cameraIdx;
	}
	for (size_t meshIdx = 0; meshIdx < parsedGraph.meshes.size(); meshIdx++) {
		parsedGraph.meshes[meshIdx].index = meshIdx;
		if (parsedGraph.meshes[meshIdx].material.has_value()) {
			std::map<int, int>::iterator mapppedMat = 
				jsonIdToMaterialId.find(*parsedGraph.meshes[meshIdx].material);
			if (mapppedMat == jsonIdToMaterialId.end()) {
				throw std::runtime_error("ERROR: Unable to find material id for mesh in Parser.");
			}
			parsedGraph.meshes[meshIdx].material = mapppedMat->second;
		}
	}

	std::chrono::high_resolution_clock::time_point end =
		std::chrono::high_resolution_clock::now();
	if (verbose) std::cout << "MEASURE parse .s72 file: " << (float)
		std::chrono::duration_cast<std::chrono::milliseconds>(
			end - start).count() << "ms" << std::endl;
	return parsedGraph;
}

//Tokenizer helper function
static std::vector <std::string> tokenizeString(std::string str) {
	std::vector<std::string> tokens;
	size_t begin = 0;
	size_t length = 0;
	while (begin + length < str.size() ) {
		if (str.at(begin + length) == ' ') {
			std::string token = str.substr(begin, length);
			tokens.push_back(token);
			begin = begin + length + 1;
			length = 0;
		}
		else {
			length++;
		}
	}
	if (begin < str.size()) {
		tokens.push_back(str.substr(begin, str.size() - begin));
	}
	return tokens;
}

static std::vector<std::string> ensureFormatting(std::vector<std::string> strings) {
	for (size_t str = 0; str < strings.size(); str++) {
		if (strings[str].at(strings[str].size() - 1) == '\r') {
			strings[str] = strings[str].substr(0, strings[str].size() - 1);
		}
	}
	return strings;
}

//Parse custom event file format for headless mode
HeadlessEvents Parser::parseEvents(std::string fileName) {
	std::vector<char> rawString = readFile(fileName);
	std::vector<std::string> rawStrings = parseIntoStrings(rawString, false);
	rawStrings = ensureFormatting(rawStrings);
	HeadlessEvents events;
	events.currentEvent = 0;
	for (std::string eventString : rawStrings) {
		std::vector<std::string> eventTokens = tokenizeString(eventString);
		if (eventTokens.size() < 2) {
			throw std::runtime_error("ERROR: Invalid line found when parsing events file in Parser.");
		}
		int time = atoi(eventTokens[0].c_str());
		std::string typeString = eventTokens[1];
		size_t typeSize = typeString.size();
		if (typeSize == 9 && !typeString.compare("AVAILABLE")) {
			Event newEvent;
			newEvent.type = EV_AVAILABLE;
			newEvent.time = time;
			events.eventsQueue.push_back(newEvent);
		}
		else if (typeSize == 4 && !typeString.compare("PLAY")) {
			if (eventTokens.size() < 4) {
				throw std::runtime_error("ERROR: Invalid PLAY line found when parsing events file in Parser.");
			}
			float playbackTime = atof(eventTokens[2].c_str());
			float playbackTrate = atof(eventTokens[3].c_str());
			Event newEvent;
			newEvent.type = EV_PLAY;
			newEvent.time = time;
			newEvent.playbackInfo.time = playbackTime;
			newEvent.playbackInfo.rate = playbackTrate;
			events.eventsQueue.push_back(newEvent);
		}
		else if (typeSize == 4 && !typeString.compare("SAVE")) {
			if (eventTokens.size() < 3) {
				throw std::runtime_error("ERROR: Invalid SAVE line found when parsing events file in Parser.");
			}
			Event newEvent;
			newEvent.type = EV_SAVE;
			newEvent.time = time;
			newEvent.saveName = eventTokens[2];
			events.eventsQueue.push_back(newEvent);
		}
		else if (typeSize == 4 && !typeString.compare("MARK")) {
			if(eventTokens.size() < 3) {
				throw std::runtime_error("ERROR: Invalid MARK line found when parsing events file in Parser.");
			}
			std::string description;
			for (size_t whichStr = 2; whichStr < eventTokens.size(); whichStr++) {
				description = description.append(eventTokens[whichStr]).append(" ");
			}
			Event newEvent;
			newEvent.type = EV_MARK;
			newEvent.markDescription = description;
			newEvent.time = time;
			events.eventsQueue.push_back(newEvent);
		}
		else {
			std::cout << "WARNING: Unsupported event found when parsing events file in Parser." << std::endl;
		}
	}
	return events;
}