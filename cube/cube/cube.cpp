// cube.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <vector>
#include <algorithm>
#include <string>
#define float_rand ((float)(rand()) / (float)(RAND_MAX))
#define PI 3.1415926535897
#define EPS 0.00000001


void cubeError() {
	throw std::runtime_error("Invalid arguments. Application must at least be"
		+ std::string(" run with cube in.png --lambertian out.png ")
		+ std::string(" or with cube in.png --ggx out.png\n")
		+ std::string("The optional arguments may also follow the original arguments:\n")
		+ std::string("'--face x y' to indicate the resolution desired for each cubemap face\n")
		+ std::string("'--samples size' to indicate number of monte carlo samples."));
}


struct float_3 {
	float x = 0;
	float y = 0;
	float z = 0;
	float_3() {}
	float_3(float a, float b, float c) {
		x = a;
		y = b;
		z = c;
	}

	float norm() {
		return sqrt(x * x + y * y + z * z);
	}
	float_3 normalize() {
		float_3 newVec;
		float norm_f = norm();
		newVec.x = x / norm_f;
		newVec.y = y / norm_f;
		newVec.z = z / norm_f;
		return newVec;
	}
	float_3 cross(float_3 b) {
		return float_3(y * b.z - z * b.y, z * b.x - x * b.z, x * b.y - y * b.x);
	}
	float_3 operator*(float_3 b) {
		return float_3(x * b.x, y * b.y, z * b.z);
	}
	float_3 operator+(float_3 b) {
		return float_3(x + b.x, y + b.y, z + b.z);
	}
	float_3 operator/(float_3 b) {
		return float_3(x / b.x, y / b.y, z / b.z);
	}
	float_3 operator-(float_3 b) {
		return float_3(x - b.x, y - b.y, z - b.z);
	}
	float_3 operator*(float b) {
		return float_3(x * b, y * b, z * b);
	}
	float dot(float_3 b) {
		return x * b.x + y * b.y + z * b.z;
	}
};


struct Pixel {
	stbi_uc r;
	stbi_uc g;
	stbi_uc b;
	stbi_uc e;
	float_3 toColor() {
		return float_3((float)r, float(g), (float)b) * (1.f / 255.f);
	};
};

float_3 operator* (float a, float_3 b) {
	return float_3(a * b.x, a * b.y, a * b.z);
}



struct mat33 {
	float data[3][3] = { {0,0,0}, {0,0,0},{0,0,0} };
	mat33() {};
	mat33(float f) {
		data[0][0] = f;
		data[1][1] = f;
		data[2][2] = f;
	}
	mat33(float_3 x, float_3 y, float_3 z) {
		data[0][0] = x.x;
		data[0][1] = x.y;
		data[0][2] = x.z;
		data[1][0] = y.x;
		data[1][1] = y.y;
		data[1][2] = y.z;
		data[2][0] = z.x;
		data[2][1] = z.y;
		data[2][2] = z.z;
	}
	mat33 transpose() {
		mat33 newMat;
		for (int i = 0; i < 3; i++) {
			for (int j = 0; j < 3; j++) {
				newMat.data[i][j] = data[j][i];
			}
		}
		return newMat;
	}


	float_3 operator*(float_3 v) {
		float_3 prod;
		prod.x = data[0][0] * v.x + data[1][0] * v.y + data[2][0] * v.z;
		prod.y = data[0][1] * v.x + data[1][1] * v.y + data[2][1] * v.z;
		prod.z = data[0][2] * v.x + data[1][2] * v.y + data[2][2] * v.z;
		return prod;
	}
};

float_3 sampleVec(float_3 in, float_3 up, int x, int y, int width, int height) {

	//Transform final pixel into world space to get sample vec
	float_3 right = up.cross(in);
	mat33 toVec = mat33(right, up, in); //May need to transpose

	//Get vec
	float f_x = (float)x / (float)(width);
	float f_y = (float)y / (float)(height);
	float sample_x = (f_x + float_rand / (float)(width)) * 2.0 - 1.0;
	float sample_y = (f_y + float_rand / (float)(height)) * 2.0 - 1.0;
	float_3 localPos = float_3(sample_x, sample_y, 1);
	float_3 worldVec = (toVec * localPos).normalize();
	return worldVec;
}

Pixel sampleData(Pixel* rawData,int realWidth, int realHeight, float_3 worldVec) {
	
	//Turn into angles
	//https://en.wikipedia.org/wiki/Vector_fields_in_cylindrical_and_spherical_coordinates
	float theta = acos(worldVec.z);
	float phi = atan2(worldVec.y, worldVec.x);
	float realXF = (phi + PI) / 2 / PI * (float)realWidth;
	float realYF = theta / PI*(float)realHeight;
	if (realXF == realWidth) realXF--;
	if (realYF == realHeight) realYF--;
	int realX = realXF;
	int realY = realYF;
	int sampleInd = realY * realWidth + realX;
	return rawData[sampleInd];
}

void sampleFaceLam(std::vector<Pixel>& storeData, Pixel* rawData, int width, int height,
	int realWidth, int realHeight, float_3 in, float_3 up, int offset, int samples, int face) {
	int lastPercent = -1;
	for (int x = 0; x < width; x++) {
		for (int y = 0; y < height; y++) {
			int storeInd = offset + y * width + x;
			float_3 avgSample;
			float e = 0;
			for (int i = 0; i < samples; i++) {
				float_3 worldVec = sampleVec(in, up, x, y, width, height);
				Pixel sampledPixel = sampleData(rawData, realWidth, realHeight, worldVec);
				avgSample.x += (float)sampledPixel.r;
				avgSample.y += (float)sampledPixel.g;
				avgSample.z += (float)sampledPixel.b;
				e += (float)sampledPixel.e;
			}
			avgSample.x /= (float)samples;
			avgSample.y /= (float)samples;
			avgSample.z /= (float)samples;
			e /= (float)samples;
			storeData[storeInd].r = (stbi_uc)avgSample.x;
			storeData[storeInd].g = (stbi_uc)avgSample.y;
			storeData[storeInd].b = (stbi_uc)avgSample.z;
			storeData[storeInd].e = e;
			int percent = (int)((float)(x * height + y) / (float)(width * height) * 100.f / 6.f + 100.f * (face / 6.f));
			if (percent != lastPercent) {
				lastPercent = percent;
				std::cout << percent << "%\n";
			}
		}
	}
}

float_3 importanceSample(float xix, float xiy, float roughness, float_3 N) {
	N = N.normalize();
	float alpha = roughness * roughness;
	float phi = 2 * PI * xix;
	float cosA = (1 - xiy);
	float cosB = (1 + (alpha * alpha - 1) * xiy);
	float cosTheta = sqrt(cosA / cosB);
	float sinTheta = sqrt(1 - cosTheta * cosTheta);

	float_3 H;
	H.x = sinTheta * cos(phi);
	H.y = sinTheta * sin(phi);
	H.z = cosTheta;

	float_3 up = abs(N.z) < 0.9999 ? float_3(0, 0, 1) : float_3(1, 0, 0);
	float_3 tangentX = up.cross(N).normalize();
	float_3 tangentY = N.cross(tangentX).normalize();
	return tangentX * H.x + tangentY * H.y + N * H.z;
}

void sampleFaceGGX(std::vector<Pixel>& storeData, Pixel* rawData, int width, int height,
	int realWidth, int realHeight, float roughness, float_3 in, float_3 up, int offset, int samples, int face, int level, int levels) {
	int lastPercent = -1;
	for (int x = 0; x < width; x++) {
		for (int y = 0; y < height; y++) {
			int storeInd = offset + y * width + x;
			float_3 avgSample;
			float totalWeight = 0;
			float_3 v = sampleVec(in, up, x, y, width, height);
			for (int i = 0; i < samples; i++) {
				float xix = (float)i / (float)samples;
				float xiy = 1/(float) i;
				float_3 h = importanceSample(xix, xiy, roughness, v);
				float_3 l = 2 * v.dot(h) * h - v;
				float nl = std::clamp(v.dot(l), 0.f, 1.f);
				if (nl > 0) {
					Pixel samplePix = sampleData(rawData, realWidth, realHeight, l);
					float_3 sampleColor = samplePix.toColor()*nl;
					avgSample = avgSample + sampleColor;
					totalWeight += nl;
				}
			}
			avgSample = avgSample * (1/totalWeight)* 256;
			if (avgSample.x > 255.f) avgSample.x = 255.f;
			if (avgSample.y > 255.f) avgSample.y = 255.f;
			if (avgSample.z > 255.f) avgSample.z = 255.f;
			storeData[storeInd].r = (stbi_uc)avgSample.x;
			storeData[storeInd].g = (stbi_uc)avgSample.y;
			storeData[storeInd].b = (stbi_uc)avgSample.z;
			storeData[storeInd].e = 122;
			float progress = (float)(level * 6 + face) / (levels * 6.f);
			int percent = (int)((float)(x * height + y) / (float)(width * height) * 100.f / (float) levels/6.f + 100.f * progress);
			if (percent != lastPercent) {
				lastPercent = percent;
				std::cout << percent << "%\n";
			}
		}
	}
}

float subG(float k, float inDot) {
	return inDot / ((inDot) * (1 - k) + k);
}

float getG(float roughness, float nv, float nl) {
	float k = (roughness + 1) * (roughness + 1) / 8;
	return subG(k, nv) * subG(k, nl);
}

void brdf(float roughness, float nv, float& A, float& B, int samples) {
	float_3 v; v.x = sqrt(1 - nv * nv); v.y = 0; v.z = nv;

	for (int i = 0; i < samples; i++) {
		float xix = (float)i / (float)(samples - 1);
		float xiy = 1 / (float)i;
		float_3 h = importanceSample(xix, xiy, roughness, float_3(0,0,1));
		float_3 l = 2 * v.dot(h) * h - v;
		float nl = std::clamp(l.z,0.f,1.f);
		float nh = std::clamp(h.z, 0.f, 1.f);
		float vh = std::clamp(v.dot(h), 0.f, 1.f);
		if (nl > 0) {
			float G = getG(roughness, nv, nl);
			float vis = G * vh / nh / nv;
			float fc = pow(1 - vh, 5);
			A += (1 - fc) * vis;
			B += fc * vis;
		}
	}
	A /= (float)samples;
	B /= (float)samples;
	A *= 256.f; if (A > 255) A = 255;
	B *= 256.f; 
	if (B > 255) B = 255;
}

int main(int argc, char* argv[])
{
	srand(time(0));
	if (argc < 4) {
		cubeError();
	}
	std::string inFile = argv[1];
	//Further assert correct input formatting
	bool lambertian = true;
	int samples = 4;
	if (std::string(argv[2]).compare(std::string("--ggx")) == 0) {
		samples = 1024;
		lambertian = false;
	}
	else if (std::string(argv[2]).compare(std::string("--lambertian")) != 0) {
		cubeError();
	}
	std::string outFile = argv[3];
	int width = 16; int height = 16;
	int levels = 8;
	if (argc > 4) {
		if (argc == 7) {
			if (std::string(argv[4]).compare(std::string("--face")) != 0) {
				cubeError();
			}
			width = atoi(argv[5]);
			height = atoi(argv[6]);
		}
		else if (argc == 6) {

			if (std::string(argv[4]).compare(std::string("--samples")) != 0) {
				cubeError();
			}
			samples = atoi(argv[5]);
		}
		else if (argc == 9) {

			if (std::string(argv[4]).compare(std::string("--face")) != 0) {
				cubeError();
			}
			width = atoi(argv[5]);
			height = atoi(argv[6]);

			if (std::string(argv[7]).compare(std::string("--samples")) != 0) {
				cubeError();
			}
			samples = atoi(argv[8]);
		}
		else {
			cubeError();
		}
	}
	int realHeight, realWidth, n;
	Pixel* rawData = (Pixel*)(void*)stbi_load(inFile.c_str(), &realHeight, &realWidth, &n, 4);
	if (!rawData) {
		if (stbi_failure_reason()) {
			std::string failureReason = stbi_failure_reason();
			throw std::runtime_error("ERROR: Unable to load texture at path " + inFile + " with error: " + failureReason);
		}
		else {
			throw std::runtime_error("ERROR: Unable to load texture at path " + inFile + " with no stbi error");
		}
	}

	if (lambertian) {
		std::vector<Pixel>  sampledData = std::vector<Pixel>(6 * width * height);

		int faceSize = width * height;
		//Pos x
		sampleFaceLam(sampledData, rawData, width, height, realWidth, realHeight, float_3(1, 0, 0), float_3(0, 1, 0), 0, samples, 0);
		//Neg x
		sampleFaceLam(sampledData, rawData, width, height, realWidth, realHeight, float_3(-1, 0, 0), float_3(0, 1, 0), faceSize, samples, 1);
		//Pos y
		sampleFaceLam(sampledData, rawData, width, height, realWidth, realHeight, float_3(0, 1, 0), float_3(0, 0, -1), 2 * faceSize, samples, 2);
		//Neg y
		sampleFaceLam(sampledData, rawData, width, height, realWidth, realHeight, float_3(0, -1, 0), float_3(0, 0, 1), 3 * faceSize, samples, 3);
		//Pos z
		sampleFaceLam(sampledData, rawData, width, height, realWidth, realHeight, float_3(0, 0, 1), float_3(0, 1, 0), 4 * faceSize, samples, 4);
		//Neg z
		sampleFaceLam(sampledData, rawData, width, height, realWidth, realHeight, float_3(0, 0, -1), float_3(0, 1, 0), 5 * faceSize, samples, 5);

		//Write sampled data to outFile
		stbi_write_png(outFile.c_str(), width, height * 6, 4, sampledData.data(), 4 * width);
	}
	else {//ggx
		int faceSize = width * height;
		if (outFile.substr(outFile.size() - 4, 4).compare(".png")) {
			cubeError();
		}
		std::string outRoot = outFile.substr(0, outFile.size() - 3);
		std::string pngStr = ".png";
		for (int i = 0; i < levels; i++) {
			std::string levelOutFile = outRoot + std::to_string(i) + pngStr;
			float roughness = (float)i / ((float)(levels - 1));
			std::vector<Pixel>  sampledData = std::vector<Pixel>(6 * width * height);
			//Pos x
			sampleFaceGGX(sampledData, rawData, width, height, realWidth, realHeight, roughness, float_3(1, 0, 0), float_3(0, 1, 0), 0, samples, 0, i,levels);
			//Neg x
			sampleFaceGGX(sampledData, rawData, width, height, realWidth, realHeight, roughness, float_3(-1, 0, 0), float_3(0, 1, 0), faceSize, samples, 1, i,levels);
			//Pos y
			sampleFaceGGX(sampledData, rawData, width, height, realWidth, realHeight, roughness, float_3(0, 1, 0), float_3(0, 0, -1), 2 * faceSize, samples, 2, i,levels);
			//Neg y
			sampleFaceGGX(sampledData, rawData, width, height, realWidth, realHeight, roughness, float_3(0, -1, 0), float_3(0, 0, 1), 3 * faceSize, samples, 3, i,levels);
			//Pos z
			sampleFaceGGX(sampledData, rawData, width, height, realWidth, realHeight, roughness, float_3(0, 0, 1), float_3(0, 1, 0), 4 * faceSize, samples, 4, i,levels);
			//Neg z
			sampleFaceGGX(sampledData, rawData, width, height, realWidth, realHeight, roughness, float_3(0, 0, -1), float_3(0, 1, 0), 5 * faceSize, samples, 5, i,levels);
		
			//Write sampled data to outFile
			stbi_write_png(levelOutFile.c_str(), width, height * 6, 4, sampledData.data(), 4 * width);
		}
		//Generate LUT for width x height	
		{
			std::string lutOutFile = "LUT" + std::to_string(width) + "x" + std::to_string(height) + pngStr;
			std::vector<Pixel>  sampledData = std::vector<Pixel>(6 * width * height);

			int lastPercent = -1;
			for (int x = 0; x < width; x++) {
				for (int y = 0; y < height; y++) {
					float roughness = (float)x / (float)(width - 1);
					float nv = (float)y / (float)(height - 1);
					float A = 0; float B = 0;
					brdf(roughness, nv, A, B, samples);
					sampledData[y * width + x].r = A;
					sampledData[y * width + x].g = B;
					sampledData[y * width + x].b = 0;
					sampledData[y * width + x].e = 255;
					int percent = (int)((float)(x * height + y) / (float)(width * height) * 100.f);
					if (percent != lastPercent) {
						lastPercent = percent;
						std::cout << percent << "%\n";
					}
				}
			}

			stbi_write_png(lutOutFile.c_str(), width, height, 4, sampledData.data(), 4 * width);
		}

	}
}