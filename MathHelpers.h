#pragma once
#include <cstdint>
#include "string"
#include "array"
#include "vector"
#include "iostream"
#include "exception"

#define uint64_2 vec2<uint64_t>
#define uint64_3 vec3<uint64_t>
#define uint64_4 vec4<uint64_t>
#define uint32_2 vec2<uint32_t>
#define uint32_3 vec3<uint32_t>
#define uint32_4 vec4<uint32_t>
#define uint16_2 vec2<uint16_t>
#define uint16_3 vec3<uint16_t>
#define uint16_4 vec4<uint16_t>
#define uint8_2 vec2<uint8_t>
#define uint8_3 vec3<uint8_t>
#define uint8_4 vec4<uint8_t>
#define int64_2 vec2<int64_t>
#define int64_3 vec3<int64_t>
#define int64_4 vec4<int64_t>
#define int32_2 vec2<int32_t>
#define int32_3 vec3<int32_t>
#define int32_4 vec4<int32_t>
#define int16_2 vec2<int16_t>
#define int16_3 vec3<int16_t>
#define int16_4 vec4<int16_t>
#define int8_2 vec2<int8_t>
#define int8_3 vec3<int8_t>
#define int8_4 vec4<uint8_t>
#define float_2 vec2<float_t>
#define float_3 vec3<float_t>
#define float_4 vec4<float_t>
#define double_2 vec2<double>
#define double_3 vec3<double>
#define double_4 vec4<double>
#define longdouble_2 vec2<long double>
#define longdouble_3 vec3<long double>
#define longdouble_4 vec4<long double>
template<typename T> struct vec2;
template<typename T> struct vec3;
template<typename T> struct vec4;
template<typename T> struct mat22;
template<typename T> struct mat23;
template<typename T> struct mat24;
template<typename T> struct mat32;
template<typename T> struct mat33;
template<typename T> struct mat34;
template<typename T> struct mat42;
template<typename T> struct mat43;
template<typename T> struct mat44;
#define PI 3.1415926535897
#define EPS 0.00000001

template<typename T> T radians(T angle) {
	return angle * (T)(2 * PI) / 360;
}

template<typename T> struct vec2 {
	T x;
	T y;
	vec2<T>(T x_ = 0, T y_ = 0) {
		x = x_;
		y = y_;
	}
	vec2<T>(vec3<T> a) {
		x = a.x;
		y = a.y;
	}
	vec2<T>(vec4<T> a) {
		x = a.x;
		y = a.y;
	}
	vec2<T>(T a) {
		x = a; y = a;
	}
	T operator[](int index) {
		if (index == 0) return x;
		return y;
	}
	vec4<T> operator+(vec4<T> b) {
		return vec4<T>(x + b.x, y + b.y, b.z, b.w);
	}
	vec4<T> operator-(vec4<T> b) {
		return vec4<T>(x - b.x, y - b.y, -b.z, -b.w);
	}
	vec4<T> operator/(vec4<T> b) {
		return vec4<T>(x / b.x, y / b.y, 0, 0);
	}
	vec4<T> operator*(vec4<T> b) {
		return vec4<T>(x * b.x, y * b.y, 0, 0);
	}
	vec3<T> operator+(vec3<T> b) {
		return vec3<T>(x + b.x, y + b.y, b.z);
	}
	vec3<T> operator-(vec3<T> b) {
		return vec3<T>(x - b.x, y - b.y, -b.z);
	}
	vec3<T> operator/(vec3<T> b) {
		return vec3<T>(x / b.x, y / b.y, 0);
	}
	vec3<T> operator*(vec3<T> b) {
		return vec3<T>(x * b.x, y * b.y, 0);
	}
	vec2<T> operator+(vec2<T> b) {
		return vec2<T>(x + b.x, y + b.y);
	}
	vec2<T> operator-(vec2<T> b) {
		return vec2<T>(x - b.x, y - b.y);
	}
	vec2<T> operator/(vec2<T> b) {
		return vec2<T>(x / b.x, y / b.y);
	}
	vec2<T> operator*(vec2<T> b) {
		return vec2<T>(x * b.x, y * b.y);
	}
	vec2<T> operator+(T b) {
		return vec2<T>(x + b, y + b);
	}
	vec2<T> operator-(T b) {
		return vec2<T>(x - b.x, y - b.y);
	}
	vec2<T> operator/(T b) {
		return vec2<T>(x / b, y / b);
	}
	vec2<T> operator*(T b) {
		return vec2<T>(x * b, y * b);
	}
	T norm() {
		return static_cast<T>(sqrt(x * x + y * y));
	}
	vec2<T> normalize() {
		T n = norm();
		return *this / vec2<T>(n, n);
	}
	T dot(vec2<T> b) {
		return x * b.x + y * b.y;
	}
	vec3<T> cross(vec2<T> b) {
		return (0, 0, x * b.y - y * b.x);
	}
};

template<typename T> struct vec3 {
	T x;
	T y;
	T z;
	vec3<T>(T a) {
		x = a; y = a; z = a;
	}
	vec3<T>(T x_ = 0, T y_ = 0, T z_ = 0) {
		x = x_;
		y = y_;
		z = z_;
	}
	vec3<T>(vec4<T> a) {
		x = a.x; y = a.y; z = a.z;
	}
	T operator[](int index) {
		if (index == 0) return x;
		if (index == 1) return y;
		return z;
	}
	vec4<T> operator+(vec4<T> b) {
		return vec4<T>(x + b.x, y + b.y, b.z, b.w);
	}
	vec4<T> operator-(vec4<T> b) {
		return vec4<T>(x - b.x, y - b.y, -b.z, -b.w);
	}
	vec4<T> operator/(vec4<T> b) {
		return vec4<T>(x / b.x, y / b.y, z / b.z, 0);
	}
	vec4<T> operator*(vec4<T> b) {
		return vec4<T>(x * b.x, y * b.y, z * b.z, 0);
	}
	vec3<T> operator+(vec3<T> b) {
		return vec3<T>(x + b.x, y + b.y, z + b.z);
	}
	vec3<T> operator-(vec3<T> b) {
		return vec3<T>(x - b.x, y - b.y, z - b.z);
	}
	vec3<T> operator/(vec3<T> b) {
		return vec3<T>(x / b.x, y / b.y, z / b.z);
	}
	vec3<T> operator*(vec3<T> b) {
		return vec3<T>(x * b.x, y * b.y, z * b.z);
	}
	vec3<T> operator+(T b) {
		return vec3<T>(x + b, y + b, z + b);
	}
	vec3<T> operator-(T b) {
		return vec3<T>(x - b, y - b, z - b);
	}
	vec3<T> operator/(T b) {
		return vec3<T>(x / b, y / b, z / b);
	}
	vec3<T> operator*(T b) {
		return vec3<T>(x * b, y * b, z * b);
	}
	T norm() {
		return static_cast<T>(sqrt(x * x + y * y + z * z));
	}
	vec3<T> normalize() {
		vec3<T> v = *this;
		vec3<T> u = vec3<T>(norm(), norm(), norm());
		return v / u;
	}
	T dot(vec3<T> b) {
		return x * b.x + y * b.y + z * b.z;
	}
	vec3<T> cross(vec3<T> b) {
		return vec3<T>(y * b.z - z * b.y, z * b.x - x * b.z, x * b.y - y * b.x);
	}
};

template<typename T> struct vec4 {
	T x;
	T y;
	T z;
	T w;
	vec4<T>(T x_ = 0, T y_ = 0, T z_ = 0, T w_ = 1) { //Init w to 1?
		x = x_;
		y = y_;
		z = z_;
		w = w_;
	}
	vec4<T>(T a) {
		x = a; y = a; z = a; w = a;
	}
	vec4<T>(vec3<T> v) {
		x = v.x; y = v.y; z = v.z; w = 1;
	}
	vec4<T>(vec3<T> v, T _w) {
		x = v.x; y = v.y; z = v.z; w = _w;
	}
	T operator[](int index) {
		if (index == 0) return x;
		if (index == 1) return y;
		if (index == 2) return z;
		return w;
	}
	vec4<T> operator+(vec4<T> b) {
		return vec4<T>(x + b.x, y + b.y, z + b.z, w + b.w);
	}
	vec4<T> operator-(vec4<T> b) {
		return vec4<T>(x - b.x, y - b.y, z - b.z, w - b.w);
	}
	vec4<T> operator/(vec4<T> b) {
		return vec4<T>(x / b.x, y / b.y, z / b.z, w / b.w);
	}
	vec4<T> operator*(vec4<T> b) {
		return vec4<T>(x * b.x, y * b.y, z * b.z, w * b.w);
	}
	vec4<T> operator+(T b) {
		return vec4<T>(x + b, y + b, z + b, w + b);
	}
	vec4<T> operator-(T b) {
		return vec4<T>(x - b, y - b, z - b, w - b);
	}
	vec4<T> operator/(T b) {
		return vec4<T>(x / b, y / b, z / b, w / b);
	}
	vec4<T> operator*(T b) {
		return vec4<T>(x * b, y * b, z * b, w * b);
	}
	T norm() {
		return static_cast<T>(sqrt(x * x + y * y + z * z + w * w));
	}
	vec4<T> normalize() {
		T n = norm();
		return *this / vec4<T>(n, n, n, n);
	}
	T dot(vec4<T> b) {
		return x * b.x + y * b.y + z * b.z + w * b.w;
	}
};

template<typename T, int n> struct vecN {
	T data[n];
	vecN<T, n>(T x) {
		for (int i = 0; i < n; i++) {
			data[i] = x;
		}
	}
	vecN<T, n>(T* x) {
		for (int i = 0; i < n; i++) {
			data[i] = x[i];
		}
	}
	vecN<T, n>(std::vector<T> x) {
		if (x.size() < n) {
			throw std::runtime_error("ERROR: Do not supply a vector of size " << n << " with a vector of a smaller size " << x.size() << ".");
		}
		for (int i = 0; i < n; i++) {
			data[i] = x[i];
		}
	}
	vecN<T, n>(std::array<T, n> x) {
		for (int i = 0; i < n; i++) {
			data[i] = x[i];
		}
	}
	vecN<T, n> operator+(vecN<T, n> b) {
		vecN<T, n> retVec = this;
		for (int i = 0; i < n; i++) {
			retVec[i] += b[i];
		}
		return retVec;
	}
	vecN<T, n> operator-(vecN<T, n> b) {
		vecN<T, n> retVec = this;
		for (int i = 0; i < n; i++) {
			retVec[i] -= b[i];
		}
		return retVec;
	}
	vecN<T, n> operator/(vecN<T, n> b) {
		vecN<T, n> retVec = this;
		for (int i = 0; i < n; i++) {
			retVec[i] /= b[i];
		}
		return retVec;
	}
	vecN<T, n> operator*(vecN<T, n> b) {
		vecN<T, n> retVec = this;
		for (int i = 0; i < n; i++) {
			retVec[i] *= b[i];
		}
		return retVec;
	}
};

template<typename T> struct mat22 {
	T data[2][2] = { {0,0}, {0,0} };
	mat22<T>() {
	}
	mat22<T>(T x) {
		for (int i = 0; i < 2; i++) {
			data[i][i] = x;
		}
	}
	mat22<T>(
		T x00, T x01,
		T x10, T x11
	) {
		data = {
			{x00,x01},
			{x10,x11}
		};
	}
	mat22<T>(vec2<T> x0, vec2<T> x1) {
		data[0] = static_cast<T*>(&x0);
		data[1] = static_cast<T*>(&x1);
	}
	mat22<T> identity() {
		return mat22<T>(1);
	}
	mat22<T> transpose() {
		return (
			data[0][0], data[1][0],
			data[0][1], data[1][1]
			);
	}
	mat22<T> operator+(mat22<T> b) {
		return (data[0] + b[0], data[2] + b[1]);
	}
	mat22<T> operator*(mat22<T> b) {
		T x00 = b[0][0] * data[0][0] + b[0][1] * data[1][0];
		T x01 = b[0][0] * data[0][1] + b[0][1] * data[1][1];
		T x10 = b[1][0] * data[0][0] + b[1][1] * data[1][0];
		T x11 = b[1][0] * data[0][1] + b[1][1] * data[1][1];
		return mat22<T>(x00, x01, x10, x11);
	}
	mat23<T> operator*(mat23<T> b) {
		T x00 = b[0][0] * data[0][0] + b[0][1] * data[1][0];
		T x01 = b[0][0] * data[0][1] + b[0][1] * data[1][1];
		T x02 = b[0][0] * data[0][2] + b[0][1] * data[1][2];
		T x10 = b[1][0] * data[0][0] + b[1][1] * data[1][0];
		T x11 = b[1][0] * data[0][1] + b[1][1] * data[1][1];
		T x12 = b[1][0] * data[0][2] + b[1][1] * data[1][2];
		return mat23<T>(x00, x01, x02, x10, x11, x12);
	}
	mat24<T> operator*(mat24<T> b) {
		T x00 = b[0][0] * data[0][0] + b[0][1] * data[1][0];
		T x01 = b[0][0] * data[0][1] + b[0][1] * data[1][1];
		T x02 = b[0][0] * data[0][2] + b[0][1] * data[1][2];
		T x03 = b[0][0] * data[0][3] + b[0][1] * data[1][3];
		T x10 = b[1][0] * data[0][0] + b[1][1] * data[1][0];
		T x11 = b[1][0] * data[0][1] + b[1][1] * data[1][1];
		T x12 = b[1][0] * data[0][2] + b[1][1] * data[1][2];
		T x13 = b[1][0] * data[0][3] + b[1][1] * data[1][3];
		return mat24<T>(x00, x01, x02, x03, x10, x11, x12, x13);
	}
};

template<typename T> struct mat23 {
	T data[2][3] = { {0,0,0}, {0,0,0} };
	mat23<T>() {
	}
	mat23<T>(T x) {
		for (int i = 0; i < 2; i++) {
			data[i][i] = x;
		}
	}
	mat23<T>(
		T x00, T x01, T x02,
		T x10, T x11, T x12
	) {
		data[0][0] = x00;
		data[0][1] = x01;
		data[0][2] = x02;
		data[1][0] = x10;
		data[1][1] = x11;
		data[1][2] = x12;
	}
	mat23<T>(vec3<T> x0, vec3<T> x1) {
		data[0] = static_cast<T*>(&x0);
		data[1] = static_cast<T*>(&x1);
	}
	mat23<T> identity() {
		return mat23<T>(1);
	}
	mat32<T> transpose() {
		return (
			data[0][0], data[1][0],
			data[0][1], data[1][1],
			data[0][2], data[1][2]
			);
	}
	mat23<T> operator+(mat23<T> b) {
		return (data[0] + b[0], data[2] + b[1]);
	}
	mat22<T> operator*(mat32<T> b) {
		T x00 = b[0][0] * data[0][0] + b[0][1] * data[1][0] + b[0][2] * data[2][0];
		T x01 = b[0][0] * data[0][1] + b[0][1] * data[1][1] + b[0][2] * data[2][1];
		T x10 = b[1][0] * data[0][0] + b[1][1] * data[1][0] + b[1][2] * data[2][0];
		T x11 = b[1][0] * data[0][1] + b[1][1] * data[1][1] + b[1][2] * data[2][1];
		return mat22<T>(x00, x01, x10, x11);
	}
	mat23<T> operator*(mat33<T> b) {
		T x00 = b[0][0] * data[0][0] + b[0][1] * data[1][0] + b[0][2] * data[2][0];
		T x01 = b[0][0] * data[0][1] + b[0][1] * data[1][1] + b[0][2] * data[2][1];
		T x02 = b[0][0] * data[0][2] + b[0][1] * data[1][2] + b[0][2] * data[2][2];
		T x10 = b[1][0] * data[0][0] + b[1][1] * data[1][0] + b[1][2] * data[2][0];
		T x11 = b[1][0] * data[0][1] + b[1][1] * data[1][1] + b[1][2] * data[2][1];
		T x12 = b[1][0] * data[0][2] + b[1][1] * data[1][2] + b[1][2] * data[2][2];
		return mat23<T>(x00, x01, x02, x10, x11, x12);
	}
	mat24<T> operator*(mat34<T> b) {
		T x00 = b[0][0] * data[0][0] + b[0][1] * data[1][0] + b[0][2] * data[2][0];
		T x01 = b[0][0] * data[0][1] + b[0][1] * data[1][1] + b[0][2] * data[2][1];
		T x02 = b[0][0] * data[0][2] + b[0][1] * data[1][2] + b[0][2] * data[2][2];
		T x03 = b[0][0] * data[0][3] + b[0][1] * data[1][3] + b[0][2] * data[2][3];
		T x10 = b[1][0] * data[0][0] + b[1][1] * data[1][0] + b[1][2] * data[2][0];
		T x11 = b[1][0] * data[0][1] + b[1][1] * data[1][1] + b[1][2] * data[2][1];
		T x12 = b[1][0] * data[0][2] + b[1][1] * data[1][2] + b[1][2] * data[2][2];
		T x13 = b[1][0] * data[0][3] + b[1][1] * data[1][3] + b[1][2] * data[2][3];
		return mat24<T>(x00, x01, x02, x03, x10, x11, x12, x13);
	}
};

template<typename T> struct mat24 {
	T data[2][4] = { {0,0,0,0}, {0,0,0,0} };
	mat24<T>() {
	}
	mat24<T>(T x) {
		for (int i = 0; i < 2; i++) {
			data[i][i] = x;
		}
	}
	mat24<T>(
		T x00, T x01, T x02, T x03,
		T x10, T x11, T x12, T x13
	) {
		data = {
			{x00,x01,x02, x03},
			{x10,x11, x12, x13}
		};
	}
	mat24<T>(vec4<T> x0, vec4<T> x1) {
		data[0] = static_cast<T*>(&x0);
		data[1] = static_cast<T*>(&x1);
	}
	mat24<T> identity() {
		return mat24<T>(1);
	}
	mat42<T> transpose() {
		return (
			data[0][0], data[1][0],
			data[0][1], data[1][1],
			data[0][2], data[1][2],
			data[0][3], data[1][3]);
	}
	mat24<T> operator+(mat23<T> b) {
		return (data[0] + b[0], data[2] + b[1]);
	}
	mat22<T> operator*(mat42<T> b) {
		T x00 = b[0][0] * data[0][0] + b[0][1] * data[1][0] + b[0][2] * data[2][0] + b[0][3] * data[3][0];
		T x01 = b[0][0] * data[0][1] + b[0][1] * data[1][1] + b[0][2] * data[2][1] + b[0][3] * data[3][1];
		T x10 = b[1][0] * data[0][0] + b[1][1] * data[1][0] + b[1][2] * data[2][0] + b[1][3] * data[3][0];
		T x11 = b[1][0] * data[0][1] + b[1][1] * data[1][1] + b[1][2] * data[2][1] + b[1][3] * data[3][1];
		return mat22<T>(x00, x01, x10, x11);
	}
	mat23<T> operator*(mat43<T> b) {
		T x00 = b[0][0] * data[0][0] + b[0][1] * data[1][0] + b[0][2] * data[2][0] + b[0][3] * data[3][0];
		T x01 = b[0][0] * data[0][1] + b[0][1] * data[1][1] + b[0][2] * data[2][1] + b[0][3] * data[3][1];
		T x02 = b[0][0] * data[0][2] + b[0][1] * data[1][2] + b[0][2] * data[2][2] + b[0][3] * data[3][2];
		T x10 = b[1][0] * data[0][0] + b[1][1] * data[1][0] + b[1][2] * data[2][0] + b[1][3] * data[3][0];
		T x11 = b[1][0] * data[0][1] + b[1][1] * data[1][1] + b[1][2] * data[2][1] + b[1][3] * data[3][1];
		T x12 = b[1][0] * data[0][2] + b[1][1] * data[1][2] + b[1][2] * data[2][2] + b[1][3] * data[3][2];
		return mat23<T>(x00, x01, x02, x10, x11, x12);
	}
	mat24<T> operator*(mat44<T> b) {
		T x00 = b[0][0] * data[0][0] + b[0][1] * data[1][0] + b[0][2] * data[2][0] + b[0][3] * data[3][0];
		T x01 = b[0][0] * data[0][1] + b[0][1] * data[1][1] + b[0][2] * data[2][1] + b[0][3] * data[3][1];
		T x02 = b[0][0] * data[0][2] + b[0][1] * data[1][2] + b[0][2] * data[2][2] + b[0][3] * data[3][2];
		T x03 = b[0][0] * data[0][3] + b[0][1] * data[1][3] + b[0][2] * data[2][3] + b[0][3] * data[3][3];
		T x10 = b[1][0] * data[0][0] + b[1][1] * data[1][0] + b[1][2] * data[2][0] + b[1][3] * data[3][0];
		T x11 = b[1][0] * data[0][1] + b[1][1] * data[1][1] + b[1][2] * data[2][1] + b[1][3] * data[3][1];
		T x12 = b[1][0] * data[0][2] + b[1][1] * data[1][2] + b[1][2] * data[2][2] + b[1][3] * data[3][2];
		T x13 = b[1][0] * data[0][3] + b[1][1] * data[1][3] + b[1][2] * data[2][3] + b[1][3] * data[3][3];
		return mat24<T>(x00, x01, x02, x03, x10, x11, x12, x13);
	}
};

template<typename T> struct mat32 {
	T data[3][2] = { {0,0}, {0,0}, {0,0} };
	mat32<T>() {
	}
	mat32<T>(T x) {
		for (int i = 0; i < 2; i++) {
			data[i][i] = x;
		}
	}
	mat32<T>(
		T x00, T x01,
		T x10, T x11,
		T x20, T x21
	) {
		data = {
			{x00,x01},
			{x10,x11},
			{x20,x21}
		};
	}
	mat32<T>(vec2<T> x0, vec2<T> x1, vec2<T> x2) {
		data[0] = static_cast<T*>(&x0);
		data[1] = static_cast<T*>(&x1);
		data[2] = static_cast<T*>(&x2);
	}
	mat32<T> identity() {
		return mat32<T>(1);
	}
	mat23<T> transpose() {
		return (
			data[0][0], data[1][0], data[2][0],
			data[0][1], data[1][1], data[2][1]
			);
	}
	mat32<T> operator+(mat32<T> b) {
		return (data[0] + b[0], data[1] + b[1], data[2] + b[2]);
	}
	mat32<T> operator*(mat22<T> b) {
		T x00 = b[0][0] * data[0][0] + b[0][1] * data[1][0];
		T x01 = b[0][0] * data[0][1] + b[0][1] * data[1][1];
		T x10 = b[1][0] * data[0][0] + b[1][1] * data[1][0];
		T x11 = b[1][0] * data[0][1] + b[1][1] * data[1][1];
		T x20 = b[2][0] * data[0][0] + b[2][1] * data[1][0];
		T x21 = b[2][0] * data[0][1] + b[2][1] * data[1][1];
		return mat32<T>(x00, x01, x10, x11, x20, x21);
	}
	mat33<T> operator*(mat23<T> b) {
		T x00 = b[0][0] * data[0][0] + b[0][1] * data[1][0];
		T x01 = b[0][0] * data[0][1] + b[0][1] * data[1][1];
		T x02 = b[0][0] * data[0][2] + b[0][1] * data[1][2];
		T x10 = b[1][0] * data[0][0] + b[1][1] * data[1][0];
		T x11 = b[1][0] * data[0][1] + b[1][1] * data[1][1];
		T x12 = b[1][0] * data[0][2] + b[1][1] * data[1][2];
		T x20 = b[2][0] * data[0][0] + b[2][1] * data[1][0];
		T x21 = b[2][0] * data[0][1] + b[2][1] * data[1][1];
		T x22 = b[2][0] * data[0][2] + b[2][1] * data[1][2];
		return mat33<T>(x00, x01, x02, x10, x11, x12, x20, x21, x22);
	}
	mat34<T> operator*(mat24<T> b) {
		T x00 = b[0][0] * data[0][0] + b[0][1] * data[1][0];
		T x01 = b[0][0] * data[0][1] + b[0][1] * data[1][1];
		T x02 = b[0][0] * data[0][2] + b[0][1] * data[1][2];
		T x03 = b[0][0] * data[0][3] + b[0][1] * data[1][3];
		T x10 = b[1][0] * data[0][0] + b[1][1] * data[1][0];
		T x11 = b[1][0] * data[0][1] + b[1][1] * data[1][1];
		T x12 = b[1][0] * data[0][2] + b[1][1] * data[1][2];
		T x13 = b[1][0] * data[0][3] + b[1][1] * data[1][3];
		T x20 = b[2][0] * data[0][0] + b[2][1] * data[1][0];
		T x21 = b[2][0] * data[0][1] + b[2][1] * data[1][1];
		T x22 = b[2][0] * data[0][2] + b[2][1] * data[1][2];
		T x23 = b[2][0] * data[0][3] + b[2][1] * data[1][3];
		return mat34<T>(x00, x01, x02, x03, x10, x11, x12, x13, x20, x21, x22, x23);
	}
};

template<typename T> struct mat33 {
	T data[3][3] = { {0,0,0}, {0,0,0} , {0,0,0} };
	mat33<T>() {
	}
	mat33<T>(T x) {
		for (int i = 0; i < 3; i++) {
			data[i][i] = x;
		}
	}
	mat33<T>(
		T x00, T x01, T x02,
		T x10, T x11, T x12,
		T x20, T x21, T x22
	) {
		data[0][0] = x00;
		data[0][1] = x01;
		data[0][2] = x02;
		data[1][0] = x10;
		data[1][1] = x11;
		data[1][2] = x12;
		data[2][0] = x20;
		data[2][1] = x21;
		data[2][2] = x22;
	}
	mat33<T>(vec3<T> x0, vec3<T> x1, vec3<T> x2) {
		data[0] = static_cast<T*>(&x0);
		data[1] = static_cast<T*>(&x1);
		data[2] = static_cast<T*>(&x2);
	}
	mat33<T>(mat44<T> m) {
		data[0][0] = m[0][0];
		data[0][1] = m[0][1];
		data[0][2] = m[0][2];
		data[1][0] = m[1][0];
		data[1][1] = m[1][1];
		data[1][2] = m[1][2];
		data[2][0] = m[2][0];
		data[2][1] = m[2][1];
		data[2][2] = m[2][2];
	}
	mat33<T> identity() {
		return mat33<T>(1);
	}
	mat33<T> transpose() {
		return (
			data[0][0], data[1][0], data[2][0],
			data[0][1], data[1][1], data[2][1],
			data[0][2], data[1][2], data[2][2]
			);
	}

	T determinate() {
		T retVal = data[0][0] * data[1][1] * data[2][2] + data[0][1] * data[1][2] * data[2][0] + data[0][2] * data[1][0] * data[2][1]
			- data[0][2] * data[1][1] * data[2][0] - data[0][1] * data[1][0] * data[2][2] - data[0][0] * data[1][2] * data[2][1];
		return retVal;
	}

	vec3<T> operator[](int i) {
		vec3<T> ret;
		ret.x = data[i][0];
		ret.y = data[i][1];
		ret.z = data[i][2];
		return ret;
	}
	mat33<T> operator+(mat33<T> b) {
		return (data[0] + b[0], data[1] + b[1], data[2] + b[2]);
	}
	vec3<T> operator*(vec3<T> v) {
		vec3<T> prod;
		prod.x = data[0][0] * v.x + data[1][0] * v.y + data[2][0] * v.z;
		prod.y = data[0][1] * v.x + data[1][1] * v.y + data[2][1] * v.z;
		prod.z = data[0][2] * v.x + data[1][2] * v.y + data[2][2] * v.z;
		return prod;
	}
	mat32<T> operator*(mat32<T> b) {
		T x00 = b[0][0] * data[0][0] + b[0][1] * data[1][0] + b[0][2] * data[2][0];
		T x01 = b[0][0] * data[0][1] + b[0][1] * data[1][1] + b[0][2] * data[2][1];
		T x10 = b[1][0] * data[0][0] + b[1][1] * data[1][0] + b[1][2] * data[2][0];
		T x11 = b[1][0] * data[0][1] + b[1][1] * data[1][1] + b[1][2] * data[2][1];
		T x20 = b[2][0] * data[0][0] + b[2][1] * data[1][0] + b[2][2] * data[2][0];
		T x21 = b[2][0] * data[0][1] + b[2][1] * data[1][1] + b[2][2] * data[2][1];
		return mat32<T>(x00, x01, x10, x11, x20, x21);
	}
	mat33<T> operator*(mat33<T> b) {
		T x00 = b[0][0] * data[0][0] + b[0][1] * data[1][0] + b[0][2] * data[2][0];
		T x01 = b[0][0] * data[0][1] + b[0][1] * data[1][1] + b[0][2] * data[2][1];
		T x02 = b[0][0] * data[0][2] + b[0][1] * data[1][2] + b[0][2] * data[2][2];
		T x10 = b[1][0] * data[0][0] + b[1][1] * data[1][0] + b[1][2] * data[2][0];
		T x11 = b[1][0] * data[0][1] + b[1][1] * data[1][1] + b[1][2] * data[2][1];
		T x12 = b[1][0] * data[0][2] + b[1][1] * data[1][2] + b[1][2] * data[2][2];
		T x20 = b[2][0] * data[0][0] + b[2][1] * data[1][0] + b[2][2] * data[2][0];
		T x21 = b[2][0] * data[0][1] + b[2][1] * data[1][1] + b[2][2] * data[2][1];
		T x22 = b[2][0] * data[0][2] + b[2][1] * data[1][2] + b[2][2] * data[2][2];
		return mat33<T>(x00, x01, x02, x10, x11, x12, x20, x21, x22);
	}
	mat34<T> operator*(mat34<T> b) {
		T x00 = b[0][0] * data[0][0] + b[0][1] * data[1][0] + b[0][1] * data[2][0];
		T x01 = b[0][0] * data[0][1] + b[0][1] * data[1][1] + b[0][1] * data[2][1];
		T x02 = b[0][0] * data[0][2] + b[0][1] * data[1][2] + b[0][1] * data[2][2];
		T x03 = b[0][0] * data[0][3] + b[0][1] * data[1][3] + b[0][1] * data[2][3];
		T x10 = b[1][0] * data[0][0] + b[1][1] * data[1][0] + b[1][1] * data[2][0];
		T x11 = b[1][0] * data[0][1] + b[1][1] * data[1][1] + b[1][1] * data[2][1];
		T x12 = b[1][0] * data[0][2] + b[1][1] * data[1][2] + b[1][1] * data[2][2];
		T x13 = b[1][0] * data[0][3] + b[1][1] * data[1][3] + b[1][1] * data[2][3];
		T x20 = b[2][0] * data[0][0] + b[2][1] * data[1][0] + b[2][1] * data[2][0];
		T x21 = b[2][0] * data[0][1] + b[2][1] * data[1][1] + b[2][1] * data[2][1];
		T x22 = b[2][0] * data[0][2] + b[2][1] * data[1][2] + b[2][1] * data[2][2];
		T x23 = b[2][0] * data[0][3] + b[2][1] * data[1][3] + b[2][1] * data[2][3];
		return mat34<T>(x00, x01, x02, x03, x10, x11, x12, x13, x20, x21, x22, x23);
	}

	void print() {
		for (int i = 0; i < 3; i++) {
			for (int j = 0; j < 3; j++) {
				std::cout << data[i][j] << " ";
			}
			std::cout << std::endl;
		}
	}
};

template<typename T> struct mat34 {
	T data[3][4] = { {0,0,0,0}, {0,0,0,0},{0,0,0,0} };
	mat34<T>() {
	}
	mat34<T>(T x) {
		for (int i = 0; i < 3; i++) {
			data[i][i] = x;
		}
	}
	mat34<T>(
		T x00, T x01, T x02, T x03,
		T x10, T x11, T x12, T x13,
		T x20, T x21, T x22, T x23
	) {
		data = {
			{x00,x01,x02,x03},
			{x10,x11,x12,x13},
			{x20,x21,x22,x23} };
	}
	mat34<T>(vec3<T> x0, vec3<T> x1, vec3<T> x2) {
		data[0] = static_cast<T*>(&x0);
		data[1] = static_cast<T*>(&x1);
		data[2] = static_cast<T*>(&x2);
	}
	mat34<T> identity() {
		return mat34<T>(1);
	}
	mat43<T> transpose() {
		return (
			data[0][0], data[1][0], data[2][0],
			data[0][1], data[1][1], data[2][1],
			data[0][2], data[1][2], data[2][2],
			data[0][1], data[0][2], data[0][3]
			);
	}
	mat34<T> operator+(mat34<T> b) {
		return (data[0] + b[0], data[1] + b[1], data[2] + b[2]);
	}
	mat32<T> operator*(mat42<T> b) {
		T x00 = b[0][0] * data[0][0] + b[0][1] * data[1][0] + b[0][2] * data[2][0] + b[0][3] * data[3][0];
		T x01 = b[0][0] * data[0][1] + b[0][1] * data[1][1] + b[0][2] * data[2][1] + b[0][3] * data[3][1];
		T x10 = b[1][0] * data[0][0] + b[1][1] * data[1][0] + b[1][2] * data[2][0] + b[1][3] * data[3][0];
		T x11 = b[1][0] * data[0][1] + b[1][1] * data[1][1] + b[1][2] * data[2][1] + b[1][3] * data[3][1];
		T x20 = b[2][0] * data[0][0] + b[2][1] * data[1][0] + b[2][2] * data[2][0] + b[2][3] * data[3][0];
		T x21 = b[2][0] * data[0][1] + b[2][1] * data[1][1] + b[2][2] * data[2][1] + b[2][3] * data[3][1];
		return mat32<T>(x00, x01, x10, x11, x20, x21);
	}
	mat33<T> operator*(mat43<T> b) {
		T x00 = b[0][0] * data[0][0] + b[0][1] * data[1][0] + b[0][2] * data[2][0] + b[0][3] * data[3][0];
		T x01 = b[0][0] * data[0][1] + b[0][1] * data[1][1] + b[0][2] * data[2][1] + b[0][3] * data[3][1];
		T x02 = b[0][0] * data[0][2] + b[0][1] * data[1][2] + b[0][2] * data[2][2] + b[0][3] * data[3][2];
		T x10 = b[1][0] * data[0][0] + b[1][1] * data[1][0] + b[1][2] * data[2][0] + b[1][3] * data[3][0];
		T x11 = b[1][0] * data[0][1] + b[1][1] * data[1][1] + b[1][2] * data[2][1] + b[1][3] * data[3][1];
		T x12 = b[1][0] * data[0][2] + b[1][1] * data[1][2] + b[1][2] * data[2][2] + b[1][3] * data[3][2];
		T x20 = b[2][0] * data[0][0] + b[2][1] * data[1][0] + b[2][2] * data[2][0] + b[2][3] * data[3][0];
		T x21 = b[2][0] * data[0][1] + b[2][1] * data[1][1] + b[2][2] * data[2][1] + b[2][3] * data[3][1];
		T x22 = b[2][0] * data[0][2] + b[2][1] * data[1][2] + b[2][2] * data[2][2] + b[2][3] * data[3][2];
		return mat33<T>(x00, x01, x02, x10, x11, x12, x20, x21, x22);
	}
	mat34<T> operator*(mat44<T> b) {
		T x00 = b[0][0] * data[0][0] + b[0][1] * data[1][0] + b[0][1] * data[2][0] + b[0][3] * data[3][0];
		T x01 = b[0][0] * data[0][1] + b[0][1] * data[1][1] + b[0][1] * data[2][1] + b[0][3] * data[3][1];
		T x02 = b[0][0] * data[0][2] + b[0][1] * data[1][2] + b[0][1] * data[2][2] + b[0][3] * data[3][2];
		T x03 = b[0][0] * data[0][3] + b[0][1] * data[1][3] + b[0][1] * data[2][3] + b[0][3] * data[3][3];
		T x10 = b[1][0] * data[0][0] + b[1][1] * data[1][0] + b[1][1] * data[2][0] + b[1][3] * data[3][0];
		T x11 = b[1][0] * data[0][1] + b[1][1] * data[1][1] + b[1][1] * data[2][1] + b[1][3] * data[3][1];
		T x12 = b[1][0] * data[0][2] + b[1][1] * data[1][2] + b[1][1] * data[2][2] + b[1][3] * data[3][2];
		T x13 = b[1][0] * data[0][3] + b[1][1] * data[1][3] + b[1][1] * data[2][3] + b[1][3] * data[3][3];
		T x20 = b[2][0] * data[0][0] + b[2][1] * data[1][0] + b[2][1] * data[2][0] + b[2][3] * data[3][0];
		T x21 = b[2][0] * data[0][1] + b[2][1] * data[1][1] + b[2][1] * data[2][1] + b[2][3] * data[3][1];
		T x22 = b[2][0] * data[0][2] + b[2][1] * data[1][2] + b[2][1] * data[2][2] + b[2][3] * data[3][2];
		T x23 = b[2][0] * data[0][3] + b[2][1] * data[1][3] + b[2][1] * data[2][3] + b[2][3] * data[3][3];
		return mat34<T>(x00, x01, x02, x03, x10, x11, x12, x13, x20, x21, x22, x23);
	}
};

template<typename T> struct mat42 {
	T data[4][2] = { {0,0}, {0,0}, {0,0}, {0,0} };
	mat42<T>() {
	}
	mat42<T>(T x) {
		for (int i = 0; i < 2; i++) {
			data[i][i] = x;
		}
	}
	mat42<T>(T x00, T x01, T x10, T x11, T x20, T x21, T x30, T x31) {
		data = {
			{x00,x01},
			{x10,x11},
			{x20,x21},
			{x30,x31}
		};
	}
	mat42<T>(vec2<T> x0, vec2<T> x1, vec2<T> x2, vec2<T> x3) {
		data[0] = static_cast<T*>(&x0);
		data[1] = static_cast<T*>(&x1);
		data[2] = static_cast<T*>(&x2);
		data[3] = static_cast<T*>(&x3);
	}
	mat42<T> identity() {
		return mat42<T>(1);
	}
	mat24<T> transpose() {
		return (
			data[0][0], data[1][0], data[2][0], data[3][0],
			data[0][1], data[1][1], data[2][1], data[3][1]
			);
	}
	mat42<T> operator+(mat42<T> b) {
		return (data[0] + b[0], data[1] + b[1], data[2] + b[2], data[3] + b[3]);
	}
	mat42<T> operator*(mat22<T> b) {
		T x00 = b[0][0] * data[0][0] + b[0][1] * data[1][0];
		T x01 = b[0][0] * data[0][1] + b[0][1] * data[1][1];
		T x10 = b[1][0] * data[0][0] + b[1][1] * data[1][0];
		T x11 = b[1][0] * data[0][1] + b[1][1] * data[1][1];
		T x20 = b[2][0] * data[0][0] + b[2][1] * data[1][0];
		T x21 = b[2][0] * data[0][1] + b[2][1] * data[1][1];
		T x30 = b[3][0] * data[0][0] + b[3][1] * data[1][0];
		T x31 = b[3][0] * data[0][1] + b[3][1] * data[1][1];
		return mat42<T>(x00, x01, x10, x11, x20, x21, x30, x31);
	}
	mat43<T> operator*(mat23<T> b) {
		T x00 = b[0][0] * data[0][0] + b[0][1] * data[1][0];
		T x01 = b[0][0] * data[0][1] + b[0][1] * data[1][1];
		T x02 = b[0][0] * data[0][2] + b[0][1] * data[1][2];
		T x10 = b[1][0] * data[0][0] + b[1][1] * data[1][0];
		T x11 = b[1][0] * data[0][1] + b[1][1] * data[1][1];
		T x12 = b[1][0] * data[0][2] + b[1][1] * data[1][2];
		T x20 = b[2][0] * data[0][0] + b[2][1] * data[1][0];
		T x21 = b[2][0] * data[0][1] + b[2][1] * data[1][1];
		T x22 = b[2][0] * data[0][2] + b[2][1] * data[1][2];
		T x30 = b[3][0] * data[0][0] + b[3][1] * data[1][0];
		T x31 = b[3][0] * data[0][1] + b[3][1] * data[1][1];
		T x32 = b[3][0] * data[0][2] + b[3][1] * data[1][2];
		return mat43<T>(x00, x01, x02, x10, x11, x12, x20, x21, x22, x30, x31, x32);
	}
	mat44<T> operator*(mat24<T> b) {
		T x00 = b[0][0] * data[0][0] + b[0][1] * data[1][0];
		T x01 = b[0][0] * data[0][1] + b[0][1] * data[1][1];
		T x02 = b[0][0] * data[0][2] + b[0][1] * data[1][2];
		T x03 = b[0][0] * data[0][3] + b[0][1] * data[1][3];
		T x10 = b[1][0] * data[0][0] + b[1][1] * data[1][0];
		T x11 = b[1][0] * data[0][1] + b[1][1] * data[1][1];
		T x12 = b[1][0] * data[0][2] + b[1][1] * data[1][2];
		T x13 = b[1][0] * data[0][3] + b[1][1] * data[1][3];
		T x20 = b[2][0] * data[0][0] + b[2][1] * data[1][0];
		T x21 = b[2][0] * data[0][1] + b[2][1] * data[1][1];
		T x22 = b[2][0] * data[0][2] + b[2][1] * data[1][2];
		T x23 = b[2][0] * data[0][3] + b[2][1] * data[1][3];
		T x30 = b[3][0] * data[0][0] + b[3][1] * data[1][0];
		T x31 = b[3][0] * data[0][1] + b[3][1] * data[1][1];
		T x32 = b[3][0] * data[0][2] + b[3][1] * data[1][2];
		T x33 = b[3][0] * data[0][3] + b[3][1] * data[1][3];
		return mat44<T>(x00, x01, x02, x03, x10, x11, x12, x13, x20, x21, x22, x23, x30, x31, x32, x33);
	}
};

template<typename T> struct mat43 {
	T data[4][3] = { {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0} };
	mat43<T>() {
	}
	mat43<T>(T x) {
		for (int i = 0; i < 3; i++) {
			data[i][i] = x;
		}
	}
	mat43<T>(
		T x00, T x01, T x02,
		T x10, T x11, T x12,
		T x20, T x21, T x22,
		T x30, T x31, T x32
	) {
		data = {
			{x00,x01,x02},
			{x10,x11,x12},
			{x20,x21,x22},
			{x30,x31,x32}
		};
	}
	mat43<T>(vec3<T> x0, vec3<T> x1, vec3<T> x2, vec3<T> x3) {
		data[0] = static_cast<T*>(&x0);
		data[1] = static_cast<T*>(&x1);
		data[2] = static_cast<T*>(&x2);
		data[3] = static_cast<T*>(&x3);
	}
	mat43<T> identity() {
		return mat43<T>(1);
	}
	mat34<T> transpose() {
		return (
			data[0][0], data[1][0], data[2][0], data[3][0],
			data[0][1], data[1][1], data[2][1], data[3][1],
			data[0][2], data[1][2], data[2][2], data[3][2]);
	}
	mat43<T> operator+(mat42<T> b) {
		return (data[0] + b[0], data[1] + b[1], data[2] + b[2], data[3] + b[3]);
	}
	mat43<T> operator*(mat32<T> b) {
		T x00 = b[0][0] * data[0][0] + b[0][1] * data[1][0] + b[0][2] * data[2][0];
		T x01 = b[0][0] * data[0][1] + b[0][1] * data[1][1] + b[0][2] * data[2][1];
		T x10 = b[1][0] * data[0][0] + b[1][1] * data[1][0] + b[1][2] * data[2][0];
		T x11 = b[1][0] * data[0][1] + b[1][1] * data[1][1] + b[1][2] * data[2][1];
		T x20 = b[2][0] * data[0][0] + b[2][1] * data[1][0] + b[2][2] * data[2][0];
		T x21 = b[2][0] * data[0][1] + b[2][1] * data[1][1] + b[2][2] * data[2][1];
		T x30 = b[3][0] * data[0][0] + b[3][1] * data[1][0] + b[3][2] * data[2][0];
		T x31 = b[3][0] * data[0][1] + b[3][1] * data[1][1] + b[3][2] * data[2][1];
		return mat42<T>(x00, x01, x10, x11, x20, x21, x30, x31);
	}
	mat43<T> operator*(mat33<T> b) {
		T x00 = b[0][0] * data[0][0] + b[0][1] * data[1][0] + b[0][2] * data[2][0];
		T x01 = b[0][0] * data[0][1] + b[0][1] * data[1][1] + b[0][2] * data[2][1];
		T x02 = b[0][0] * data[0][2] + b[0][1] * data[1][2] + b[0][2] * data[2][2];
		T x10 = b[1][0] * data[0][0] + b[1][1] * data[1][0] + b[1][2] * data[2][0];
		T x11 = b[1][0] * data[0][1] + b[1][1] * data[1][1] + b[1][2] * data[2][1];
		T x12 = b[1][0] * data[0][2] + b[1][1] * data[1][2] + b[1][2] * data[2][2];
		T x20 = b[2][0] * data[0][0] + b[2][1] * data[1][0] + b[2][2] * data[2][0];
		T x21 = b[2][0] * data[0][1] + b[2][1] * data[1][1] + b[2][2] * data[2][1];
		T x22 = b[2][0] * data[0][2] + b[2][1] * data[1][2] + b[2][2] * data[2][2];
		T x30 = b[3][0] * data[0][0] + b[3][1] * data[1][0] + b[3][2] * data[2][0];
		T x31 = b[3][0] * data[0][1] + b[3][1] * data[1][1] + b[3][2] * data[2][1];
		T x32 = b[3][0] * data[0][2] + b[3][1] * data[1][2] + b[3][2] * data[2][2];
		return mat43<T>(x00, x01, x02, x10, x11, x12, x20, x21, x22, x30, x31, x32);
	}
	mat44<T> operator*(mat34<T> b) {
		mat44<T> prod = mat44<T>();
		for (int row = 0; row < 4; row++) {
			for (int col = 0; col < 4; col++) {
				prod.data[col][row] =
					data[0][row] * b[col][0] +
					data[1][row] * b[col][1] +
					data[2][row] * b[col][2];
			}
		}
		return prod;
	}
};

template<typename T> struct mat44 {
	T data[4][4] = { {0,0,0,0}, {0,0,0,0},{0,0,0,0},{0,0,0,0} };
	mat44<T>() {
	}
	mat44<T>(T x) {
		for (int i = 0; i < 4; i++) {
			data[i][i] = x;
		}
	}
	mat44<T>(
		T x00, T x01, T x02, T x03,
		T x10, T x11, T x12, T x13,
		T x20, T x21, T x22, T x23,
		T x30, T x31, T x32, T x33
	) {
		data[0][0] = x00; data[0][1] = x01; data[0][2] = x02; data[0][3] = x03;
		data[1][0] = x10; data[1][1] = x11; data[1][2] = x12; data[1][3] = x13;
		data[2][0] = x10; data[2][1] = x21; data[2][2] = x22; data[2][3] = x23;
		data[3][0] = x30; data[3][1] = x31; data[3][2] = x32; data[3][3] = x33;
	}
	mat44<T>(vec4<T> x0, vec4<T> x1, vec4<T> x2, vec4<T> x3) {
		for (int j = 0; j < 4; j++) {
			data[0][j] = x0[j];
			data[1][j] = x1[j];
			data[2][j] = x2[j];
			data[3][j] = x3[j];
		}
	}
	mat44<T> identity() {
		return mat44<T>(1);
	}
	mat44<T> transpose() {
		return mat44<T>(
			data[0][0], data[1][0], data[2][0], data[3][0],
			data[0][1], data[1][1], data[2][1], data[3][1],
			data[0][2], data[1][2], data[2][2], data[3][2],
			data[0][3], data[1][3], data[2][3], data[3][3]);
	}
	//ROW MAJOR
	static T determinateRow(mat44<T> m) {
		return
			m[0][0]*submatrixRow(m, 0, 0).determinate() -
			m[1][0]*submatrixRow(m, 1, 0).determinate() +
			m[2][0]*submatrixRow(m, 2, 0).determinate() -
			m[3][0]*submatrixRow(m, 3, 0).determinate();
	}
	//COL MAJOR
	static T determinateCol(mat44<T> m) {
		return
			m[0][0]*submatrixCol(m, 0, 0).determinate() -
			m[0][1]*submatrixCol(m, 1, 0).determinate() +
			m[0][2]*submatrixCol(m, 2, 0).determinate() -
			m[0][3]*submatrixCol(m, 3, 0).determinate();
	}
	vec4<T> operator[](int i) {
		vec4<T> ret;
		ret.x = data[i][0];
		ret.y = data[i][1];
		ret.z = data[i][2];
		ret.w = data[i][3];
		return ret;
	}
	mat44<T> operator+(mat44<T> b) {
		return (data[0] + b[0], data[1] + b[1], data[2] + b[2], data[3] + b[3]);
	}
	mat43<T> operator*(mat42<T> b) {
		T x00 = b[0][0] * data[0][0] + b[0][1] * data[1][0] + b[0][2] * data[2][0] + b[0][3] * data[3][0];
		T x01 = b[0][0] * data[0][1] + b[0][1] * data[1][1] + b[0][2] * data[2][1] + b[0][3] * data[3][1];
		T x10 = b[1][0] * data[0][0] + b[1][1] * data[1][0] + b[1][2] * data[2][0] + b[1][3] * data[3][0];
		T x11 = b[1][0] * data[0][1] + b[1][1] * data[1][1] + b[1][2] * data[2][1] + b[1][3] * data[3][1];
		T x20 = b[2][0] * data[0][0] + b[2][1] * data[1][0] + b[2][2] * data[2][0] + b[2][3] * data[3][0];
		T x21 = b[2][0] * data[0][1] + b[2][1] * data[1][1] + b[2][2] * data[2][1] + b[2][3] * data[3][1];
		T x30 = b[3][0] * data[0][0] + b[3][1] * data[1][0] + b[3][2] * data[2][0] + b[3][3] * data[3][0];
		T x31 = b[3][0] * data[0][1] + b[3][1] * data[1][1] + b[3][2] * data[2][1] + b[3][3] * data[3][1];
		return mat42<T>(x00, x01, x10, x11, x20, x21, x30, x31);
	}
	mat43<T> operator*(mat43<T> b) {
		T x00 = b[0][0] * data[0][0] + b[0][1] * data[1][0] + b[0][2] * data[2][0] + b[0][3] * data[3][0];
		T x01 = b[0][0] * data[0][1] + b[0][1] * data[1][1] + b[0][2] * data[2][1] + b[0][3] * data[3][1];
		T x02 = b[0][0] * data[0][2] + b[0][1] * data[1][2] + b[0][2] * data[2][2] + b[0][3] * data[3][2];
		T x10 = b[1][0] * data[0][0] + b[1][1] * data[1][0] + b[1][2] * data[2][0] + b[1][3] * data[3][0];
		T x11 = b[1][0] * data[0][1] + b[1][1] * data[1][1] + b[1][2] * data[2][1] + b[1][3] * data[3][1];
		T x12 = b[1][0] * data[0][2] + b[1][1] * data[1][2] + b[1][2] * data[2][2] + b[1][3] * data[3][2];
		T x20 = b[2][0] * data[0][0] + b[2][1] * data[1][0] + b[2][2] * data[2][0] + b[2][3] * data[3][0];
		T x21 = b[2][0] * data[0][1] + b[2][1] * data[1][1] + b[2][2] * data[2][1] + b[2][3] * data[3][1];
		T x22 = b[2][0] * data[0][2] + b[2][1] * data[1][2] + b[2][2] * data[2][2] + b[2][3] * data[3][2];
		T x30 = b[3][0] * data[0][0] + b[3][1] * data[1][0] + b[3][2] * data[2][0] + b[3][3] * data[3][0];
		T x31 = b[3][0] * data[0][1] + b[3][1] * data[1][1] + b[3][2] * data[2][1] + b[3][3] * data[3][1];
		T x32 = b[3][0] * data[0][2] + b[3][1] * data[1][2] + b[3][2] * data[2][2] + b[3][3] * data[3][2];
		return mat43<T>(x00, x01, x02, x10, x11, x12, x20, x21, x22, x30, x31, x32);
	}
	vec4<T> operator*(vec4<T> v) {
		vec4<T> prod;
		prod.x = data[0][0] * v.x + data[1][0] * v.y + data[2][0] * v.z + data[3][0] * v.w;
		prod.y = data[0][1] * v.x + data[1][1] * v.y + data[2][1] * v.z + data[3][1] * v.w;
		prod.z = data[0][2] * v.x + data[1][2] * v.y + data[2][2] * v.z + data[3][2] * v.w;
		prod.w = data[0][3] * v.x + data[1][3] * v.y + data[2][3] * v.z + data[3][3] * v.w;
		return prod;
	}
	mat44<T> operator*(mat44<T> b) {
		mat44<T> prod = mat44<T>();
		for (int row = 0; row < 4; row++) {
			for (int col = 0; col < 4; col++) {
				prod.data[col][row] =
					data[0][row] * b[col][0] +
					data[1][row] * b[col][1] +
					data[2][row] * b[col][2] +
					data[3][row] * b[col][3];
			}
		}
		return prod;
	}

	mat44<T> operator *(T x) {
		mat44<T> result;
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				result.data[i][j] = data[i][j] * x;
			}
		}
		return result;
	}

	mat44<T> operator /(T x) {
		mat44<T> result;
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				result.data[i][j] = data[i][j] / x;
			}
		}
		return result;
	}

	//https://www.scratchapixel.com/lessons/3d-basic-rendering/perspective-and-orthographic-projection-matrix/opengl-perspective-projection-matrix.html


	/*
		   _top
		  /|
		 / |
		/  |t      tan(fovy) = t/n
	   /   |       t = n*tan(fovy)
	  /    |
	 /fovy |
	O-------|Near
		n


	*/


	static mat44<T> perspective(T fovy, T aspect, T nearP, T farP) {
		T n = nearP; T f = farP;
		T t = tan(fovy / 2.0) * n;
		T b = -t;
		T r = t * aspect;
		T l = -r;

		return mat44<T>(
			2 * n / (r - l), 0, 0, 0,
			0, 2 * n / (t - b), 0, 0,
			0, 0, -(f + n) / (f - n), -1,
			0, 0, -f * n / (f - n), 0
		);
	}

	//ROW MAJOR
	static mat33<T> submatrixRow(mat44<T> m, int i, int j) {
		mat33<T> subM;
		for (int row = 0; row < 3; row++) {
			for (int col = 0; col < 3; col++) {
				int subRow = row; if (subRow >= i) subRow++;
				int subCol = col; if (subCol >= j) subCol++;
				subM.data[row][col] = m[subRow][subCol];
			}
		}
		return subM;
	}

	//COL MAJOR
	static mat33<T> submatrixCol(mat44<T> m, int i, int j) {
		mat33<T> subM;
		for (int row = 0; row < 3; row++) {
			for (int col = 0; col < 3; col++) {
				int subRow = row; if (subRow == i) subRow++;
				int subCol = col; if (subCol == j) subCol++;
				subM.data[col][row] = m[subCol][subRow];
			}
		}
		return subM;
	}

	//https://semath.info/src/inverse-cofactor-ex4.html
	static mat44<T> inverse(mat44<T> m) {
		//m = m.transpose();
		mat44<T> mCofactor;
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				mat33 submat = submatrixRow(m, j, i);
				T det = submat.determinate();
				mCofactor.data[i][j] = pow(-1, i + j) * det;
				continue;
			}
		}
		//TODO: Looks to me like this is a row major vs column major issue!!!!
		//And a determiante issue
		T determinate = m.determinateRow(mCofactor);
		mat44<T> mInverse = mCofactor / determinate;
		return mInverse; // mInverse.transpose();
	}

	void print() {
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				std::cout << std::to_string(data[i][j]) << " ";
			}
			std::cout << std::endl;
		}
	}

};

//Interface based on glm::quaternion but implementation entirely original unless noted otherwise
template<typename T> class quaternion {
private:
	vec3<T> _axis;
	T _angle;
public:
	quaternion<T>() {
		_angle = 1;
		_axis = vec3<T>(0, 0, 0);
	}
	quaternion<T>(vec4<T> v) {
		_angle = v[0];
		_axis.x = v[1];
		_axis.y = v[2];
		_axis.z = v[3];
	}
	quaternion<T>(T angle, T v0, T v1, T v2) {
		_angle = angle;
		_axis.x = v0;
		_axis.y = v1;
		_axis.z = v2;
	}
	quaternion<T>(T angle, vec3<T> axis) {
		_angle = angle;
		_axis = axis;
	}
	static quaternion<T> angleAxis(const T& angle, const vec3<T> axis) {
		quaternion<T> newQuaternion;
		T a2 = angle / 2;
		T s = sin(a2);
		newQuaternion._angle = cos(a2);
		newQuaternion._axis.x = axis.x * s;
		newQuaternion._axis.y = axis.y * s;
		newQuaternion._axis.z = axis.z * s;
		return newQuaternion;
	}
	vec3<T> axis() {
		return _axis;
	}
	T angle() {
		return _angle;
	}
	void setAngle(T x) {
		_angle = x;
	}
	void setAxis(vec3<T> vec) {
		_axis = vec;
	}
	quaternion<T> invert() {
		vec3<T> newAxis = _axis;
		newAxis.x *= -1;
		newAxis.y *= -1;
		newAxis.z *= -1;
		return quaternion<T>(_angle, newAxis);
	}
	static quaternion<T> normalize(quaternion<T> q) {
		T n = norm(q);
		vec3<T> vn = vec3<T>(n, n, n);
		return quaternion<T>(q.angle() / n, q.axis() / vn);
	}
	quaternion<T> normalize() {
		T n = norm();
		vec3<T> vn = vec3<T>(n, n, n);
		return quaternion<T>(_angle / n, _axis / vn);
	}
	static T norm(quaternion<T> q) {
		T a = q.angle();
		T x = q.axis().x;
		T y = q.axis().y;
		T z = q.axis().z;
		return static_cast<T>(sqrt(a * a + x * x + y * y + z * z));
	}
	T norm() {
		T a = _angle;
		T x = _axis.x;
		T y = _axis.y;
		T z = _axis.z;
		return static_cast<T>(sqrt(a * a + x * x + y * y + z * z));
	}
	//Implementation guided by https://danceswithcode.net/engineeringnotes/quaternions/quaternions.html
	vec3<T> eulerAngles() {
		T x = atan2(2 * (_angle * _axis[0] + _axis[1] * _axis[2]), _angle * _angle - _axis[0] * _axis[0] - _axis[1] * _axis[1] - _axis[2] * _axis[2]);
		T y = sin(2 * (_angle * _axis[0] + _axis[1] * _axis[2]));
		T z = atan2(2 * (_angle * _axis[0] + _axis[1] * _axis[2]), _angle * _angle + _axis[0] * _axis[0] - _axis[1] * _axis[1] - _axis[2] * _axis[2]);
		return vec3<T>(x, y, z);
	}
	//https://en.wikipedia.org/wiki/Quaternion
	quaternion<T> operator * (quaternion<T> q) {
		T angle = _angle * q.angle() - _axis[0] * q.axis()[0] - _axis[1] * q.axis()[1] - _axis[2] * q.axis()[2];
		T axis0 = _angle * q.axis()[0] + _axis[0] * q.angle() + _axis[1] * q.axis()[2] - _axis[2] * q.axis()[1];
		T axis1 = _angle * q.axis()[1] - _axis[0] * q.axis()[2] + _axis[1] * q.angle() + _axis[2] * q.axis()[0];
		T axis2 = _angle * q.axis()[2] + _axis[0] * q.axis()[1] - _axis[1] * q.axis()[0] + _axis[2] * q.angle();
		quaternion<T> retQuat; retQuat.setAngle(angle); retQuat.setAxis(vec3<T>(axis0, axis1, axis2));
		return retQuat;
	}
	//https://en.wikipedia.org/wiki/Quaternion
	quaternion<T> operator + (quaternion<T> q) {
		quaternion<T> retQuat; retQuat.setAngle(q.angle() + _angle); retQuat.setAxis(q.axis() + axis());
		return retQuat;
	}
	//https://en.wikipedia.org/wiki/Quaternions_and_spatial_rotation#Quaternion-derived_rotation_matrix
	static mat44<T> rotate(mat44<T> m, quaternion<T>q) {
		vec3<T> axis = q.axis();
		T a = q.angle();
		T s = 1;
		//Create rotation matrix rot of q such that given a point p,
		//rot*p = qpq^-1
		//column major version
		mat44<T> rot(1);
		rot.data[0][0] = 1 - 2 * s * (axis.y * axis.y + axis.z * axis.z);
		rot.data[1][0] = 2 * s * (axis.x * axis.y - axis.z * a);
		rot.data[2][0] = 2 * s * (axis.x * axis.z + axis.y * a);

		rot.data[0][1] = 2 * s * (axis.x * axis.y + axis.z * a);
		rot.data[1][1] = 1 - 2 * s * (axis.x * axis.x + axis.z * axis.z);
		rot.data[2][1] = 2 * s * (axis.y * axis.z - axis.x * a);

		rot.data[0][2] = 2 * s * (axis.x * axis.z - axis.y * a);
		rot.data[1][2] = 2 * s * (axis.y * axis.z + axis.x * a);
		rot.data[2][2] = 1 - 2 * s * (axis.x * axis.x + axis.y * axis.y);

		vec4<T> m0 = m[0];
		vec4<T> m1 = m[1];
		vec4<T> m2 = m[2];
		vec4<T> m3 = m[3];

		vec4<T> v0 = rot * m0;
		vec4<T> v1 = rot * m1;
		vec4<T> v2 = rot * m2;
		vec4<T> v3 = rot * m3;
		return mat44<T>(v0, v1, v2, v3);

	}
	static vec4<T> rotate(vec4<T> v, quaternion<T>q) {
		vec3<T> axis = q.axis();
		T a = q.angle();
		T s = 1;
		//Create rotation matrix rot of q such that given a point p,
		//rot*p = qpq^-1
		//column major version
		mat44<T> rot(1);
		rot.data[0][0] = 1 - 2 * s * (axis.y * axis.y + axis.z * axis.z);
		rot.data[1][0] = 2 * s * (axis.x * axis.y - axis.z * a);
		rot.data[2][0] = 2 * s * (axis.x * axis.z + axis.y * a);

		rot.data[0][1] = 2 * s * (axis.x * axis.y + axis.z * a);
		rot.data[1][1] = 1 - 2 * s * (axis.x * axis.x + axis.z * axis.z);
		rot.data[2][1] = 2 * s * (axis.y * axis.z - axis.x * a);

		rot.data[0][2] = 2 * s * (axis.x * axis.z - axis.y * a);
		rot.data[1][2] = 2 * s * (axis.y * axis.z + axis.x * a);
		rot.data[2][2] = 1 - 2 * s * (axis.x * axis.x + axis.y * axis.y);

		return rot * v;

	}
	static mat44<T> rotate(mat44<T> m, T a, vec3<T> v) {
		return rotate(m, angleAxis(a, v));
	}
	std::string toString() {
		return std::to_string(_angle).append(" ").append(std::to_string(_axis.x)).append(" ").append(std::to_string(_axis.y)).append(" ").append(std::to_string(_axis.z));
	}
	T dot(quaternion<T> q2) {
		return
			q2.axis().x * _axis.x +
			q2.axis().y * _axis.y +
			q2.axis().z * _axis.z +
			q2.angle() * _angle;
	}
	quaternion<T> operator*(T x) {
		quaternion<T> updatedQuat = quaternion<T>();
		updatedQuat.setAngle(_angle * x);
		updatedQuat.setAxis(_axis * x);
		return updatedQuat;
	}
};