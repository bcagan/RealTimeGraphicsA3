#pragma once
#include "MathHelpers.h"
#include <string>
#include <vector>
#include <optional>
#include "Vertex.h"
#include <map>
#include <vulkan/vulkan_core.h>


#define MAX_POOL 10000

//Struct attached to node to handle animation
enum Channel { CH_TRANSLATE, CH_ROTATE, CH_SCALE };
enum Interpolation { STEP, LINEAR, SLERP };
struct SceneDriver {
	std::string name;
	Channel channel;
	std::vector<float> times;
	std::vector<float> values;
	Interpolation interpolation;
	int index = -1;
};

//Processed driver data to be directly handled by vulkan system
struct Driver {
	std::string name;
	Channel channel;
	std::vector<float> times;
	std::vector<float> values;
	Interpolation interpolation;
	float currentRuntime = 0.0f;
	int id;
	int lastIndex = -1;

	//Process driver node to only store needed info
	static Driver fromSceneDriver(SceneDriver dr) {
		Driver newDriver;
		newDriver.name = dr.name;
		newDriver.channel = dr.channel;
		newDriver.times = dr.times;
		newDriver.values = dr.values;
		newDriver.interpolation = dr.interpolation;
		return newDriver;
	}
};

//Material structures
struct Texture {
	const unsigned char* data;
	enum {
		TYPE_2D, TYPE_CUBE
	} type;
	enum {
		FORM_LIN, FORM_RGBE
	} format;
	int x; int y; //Considered a single "block size"
	//So full resolution if 2D, 1/6 of data.size() if cube
	// 
	// 
	//In case of cube, this means that each X x Y is one cube, and there are 6
	//faces along the y axis, all width x and height y
	// data = 
	//[ .... x.... ]
	//[ .          ]
	//[ .          ]
	//[ y          ] //face 0
	//[ .          ]
	//[ .          ]
	//[ .          ]
	//--------------
	//		.
	//      .
	//      face 1
	//      .
	//      .
	//     faces2-5
	int realY;
	int mipLevels;
	bool doFree = true;
	Texture() {};
	~Texture() {};
	static Texture parseTexture(std::string path, bool cube);
};


enum LightType {
	LIGHT_NONE, LIGHT_SPHERE, LIGHT_SUN, LIGHT_SPOT
};

struct Light {
	int type;
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

	mat44<float> toWorld;
	mat44<float> toLocal;
};

struct DrawLight {
	int type;
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

enum MaterialType {
	MAT_NONE, MAT_PBR, MAT_LAM, MAT_MIR, MAT_ENV, MAT_SIM
};

struct PBR {
	struct {
		float_3 value;
		int texture;
	} albedo;
	struct {
		float value;
		int texture;
	} roughness;
	struct {
		float value;
		int texture;
	} specular;
	bool useValueAlbedo;
	bool useValueRoughness;
	bool useValueSpecular;
};

struct PBRInt {
	struct Albedo {
		float_3 value;
		std::optional<std::pair<Texture, std::string>> texture;
	};
	Albedo albedo;
	struct Roughness {
		float value;
		std::optional<std::pair<Texture, std::string>> texture;
	}; 
	Roughness roughness;
	struct Specular {
		float value;
		std::optional<std::pair<Texture, std::string>> texture;
	};
	Specular specular;
	bool useValueAlbedo;
	bool useValueRoughness;
	bool useValueSpecular;
	//https://en.cppreference.com/w/cpp/language/operators
	void set(const PBRInt& pbr) {
		useValueAlbedo = pbr.useValueAlbedo;
		useValueRoughness = pbr.useValueRoughness;
		useValueSpecular = pbr.useValueSpecular;
		if (useValueAlbedo) {
			albedo.value = pbr.albedo.value;
		}
		else {
			albedo.texture = pbr.albedo.texture;
		}
		if (useValueRoughness) {
			roughness.value = pbr.roughness.value;
		}
		else {
			roughness.texture = pbr.roughness.texture;
		}
		if (useValueSpecular) {
			specular.value = pbr.specular.value;
		}
		else {
			specular.texture = pbr.specular.texture;
		}
	}
};

struct LambertianInt {
	float_3 value;
	std::optional<std::pair<Texture, std::string>> texture;
	bool useValue;
};

struct MaterialData {
	float_3 albedo;
	int albedoTexture;
	float roughness;
	int roughnessTexture;
	float specular;
	int specularTexture;
	bool useValueAlbedo;
	bool useValueRoughness;
	bool useValueSpecular;
};

struct MaterialDataInt {
	PBRInt pbr;
	LambertianInt lambertian; //true use base color
	bool environment; //cubemap id
	bool mirror;
	bool simple;
};

struct Material {
	std::string name;
	MaterialType type;
	std::optional<int> normalMap;
	std::optional<int> displacementMap;
	MaterialData data;
	int index;
};

struct DrawMaterial {
	uint32_t useValueAlbedo;
	uint32_t useValueRoughness;
	uint32_t useValueSpecular;
	uint32_t useNormalMap = false;
	uint32_t useDisplacementMap = false;
	int normalMap;
	int displacementMap;
	int albedoTexture;
	float roughness;
	int roughnessTexture;
	float specular;
	int type;
	int specularTexture;
	float albedor;
	float albedog;
	float albedob;
};

//Intermediate material with directly referenced textures
struct MaterialInt {
	std::string name;
	MaterialType type;
	std::optional<std::pair<Texture,std::string>> normalMap;
	std::optional<std::pair<Texture, std::string>> displacementMap;
	MaterialDataInt data;
};

//Stores camera and transform information so that a graph nodes information can be
//directly updated by a driver without needing to access the graph
struct Perspective {
	float aspect;
	float vfov;
	float nearP;
	float farP;
};
struct ForAnimate {
	mat44<float> parent;
	float_3 translate;
	quaternion<float> rotation;
	float_3 scale;
};

//Finailized process nodes to be accessed if needed by Vulkan system directly
//by index, vs. through graph navigation
struct DrawNode {
	std::string name;
	int count;
	int start;
	int indexCount;
	int indexStart;
	mat44<float> transform;
	mat44<float> normalTransform;
	ForAnimate forAnimate;
	std::pair<float_3, float> boundingSphere;
	static std::pair<float_3, float> produceBoundingSphere(std::vector<Vertex>);
	std::optional<Material> material;
};

//Finallized processed camera structure to be passed to vulkan system
struct DrawCamera {
	std::string name;
	mat44<float> transform;
	mat44<float> perspective;
	Perspective perspectiveInfo;
	//For animation
	ForAnimate forAnimate;
};

//Data structures resulting from scene graph navigation not yet updated to
//be split into the required pools
class DrawListIntermediate
{
public:
	std::vector<Vertex> vertexPool;
	std::vector<uint32_t> indexPool;
	std::vector<DrawNode> drawPool;
	std::vector<DrawCamera> cameras;
	std::vector<mat44<float>> transforms;
	std::vector<mat44<float>> normalTransforms;
	std::vector < std::vector<mat44<float>>> instancedTransforms;
	std::vector < std::vector<mat44<float>>> instancedNormalTransforms;
	std::optional<mat44<float>> worldToEnvironment;
	std::vector<Driver> nodeDrivers;
	std::vector<Driver> cameraDrivers;
	std::vector<Texture> textureMaps;
	std::vector<Texture> cubeMaps;
	std::optional<Texture> environmentMap;
	std::vector<Light> lights;
};

//All the finalized processed data structures resulting from graph navigation
//To be directly passed into vulkan system

//A note on pools:
//A constant number of transforms can be loaded into the uniform buffers
//as defined in the vertex shaders. As a result, all n transforms for all
//n processed nodes can not be sent at the same time. While this system does
//Allow for multiple meshes to be parsed with as singular vertex buffer call,
//it still requires all transforms, indicies, and bounding spheres to be
//split into "sub draw-pools" as large as or smaller than the maximum
//number of transforms defined in the shader. This value can be set lower
//to minimize memory transfers at a cost of more draw calls.
class DrawList
{
public:
	std::vector<Vertex> vertexPool;
	std::vector<Vertex> instancedVertexPool;
	std::vector<std::vector<uint32_t>> indexPools;
	std::vector<std::vector<uint32_t>> instancedIndexPools;
	std::vector<std::pair<float_3, float>> instancedBoundingSpheres;
	std::vector<std::vector<DrawNode>> drawPools;
	std::vector<DrawCamera> cameras;
	std::vector<std::vector<mat44<float>>> transformPools;
	std::vector<std::vector<mat44<float>>> normalTransformPools;
	std::vector<std::vector<mat44<float>>> instancedTransformPools;
	std::vector<std::vector<mat44<float>>> instancedNormalTransformPools;
	std::vector<std::vector<mat44<float>>> environmentTransformPools;
	std::vector<std::vector<mat44<float>>> instancedEnvironmentTransformPools;
	std::vector<std::vector<DrawMaterial>> materialPools;
	std::vector<DrawMaterial> instancedMaterials;
	std::vector<int> instancedTransformIndexPools;
	std::vector<Driver> nodeDrivers;
	std::vector<Driver> cameraDrivers;
	bool useInstancing;
	std::vector<Texture> textureMaps;
	std::vector<Texture> cubeMaps;
	std::optional<Texture> environmentMap;
	std::vector<DrawLight> lights;
	std::vector<mat44<float>> worldToLights;
	std::vector<mat44<float>> worldToLightsPersp;
};

//A scene graph node parsed from a .s72 file
struct GraphNode {
	int index;
	std::string name;
	float_3 translate;
	quaternion<float> rotation;
	float_3 scale;
	std::vector<int> children;
	std::optional<int> mesh;
	std::optional<int> camera;
	std::optional<SceneDriver> translateDriver;
	std::optional<SceneDriver> rotateDriver;
	std::optional<SceneDriver> scaleDriver;
	bool hasEnvironment = false;
	std::optional<int> light;
};

//A mesh data structure parsed from both a .s72 and .b72 file
struct Mesh {
	struct Attributes {
		int offset; int stride; VkFormat format;
	};
	std::string name;
	int count;
	std::optional<std::vector<uint32_t>> indicies;
	int vertexOffset;
	int index;
	bool instanceMesh; //This mesh will be references multiple times
	std::optional<int> material;
};

//A camera data structured parsed from .s72 file
struct Camera {
	std::string name;
	Perspective perspective;
	int index;
};

//Vertex information sans the per-pool transform node
struct SceneVertex {
	float_3 pos;
	float_3 normal;
	float_4 tangent;
	float_2 texcoord;
	float_3 color;
};

//All the scene graph functions directly parsed from a .s72 file, including
//functions to further navigate and parsed into a drawlist
class SceneGraph
{
public:
	bool useInstancing;
	DrawListIntermediate navigateSceneGraphInt(int instanceSize = 0);
	DrawList navigateSceneGraph(bool verbose = false, int poolSize = MAX_POOL);
	void navigateSceneGraphMeshes(std::vector<bool>& encountered, int node);
	std::string name;
	std::vector<GraphNode> graphNodes;
	std::vector<int> roots;
	std::vector<Mesh> meshes;
	std::vector<Camera> cameras;
	std::vector<Material> materials;
	std::vector<SceneVertex> vertexPool;
	void recurseSceneGraph(
		std::vector<DrawCamera>& drawCameras,
		std::vector<Light>& drawLights,
		std::vector<Vertex>& vertices, 
		std::vector<uint32_t>& indices, 
		int node,
		mat44<float> toWorld,
		mat44<float> fromWorld,
		int& drawNode,
		std::vector<Driver>& nodeDrivers, 
		std::vector<Driver>& cameraDrivers,
		std::vector < std::vector<mat44<float>>>& instancedTransforms,
		std::vector<std::vector<mat44<float>>>& instancedNormalTransforms,
		std::vector<DrawNode>& drawNodes
	);
	std::vector<Texture> textureMaps;
	std::vector<Texture> cubeMaps;
	std::optional<Texture> environmentMap;
	std::optional<mat44<float>> worldToEnvironment;
	std::optional<mat44<float>> environmentToWorld;
	std::vector<Light> lights;
	int maxPool = MAX_POOL;
private:
	std::map<std::string, int> instancedToPool;
	std::vector<DrawLight> toDrawLights(std::vector<Light> lights);
	std::vector<DrawLight> toDrawLights(std::vector<Light> lights, std::vector<mat44<float>>& transforms);
};



