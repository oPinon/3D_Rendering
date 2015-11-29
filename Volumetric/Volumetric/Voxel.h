#pragma once

#include <stdlib.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>

using namespace std;

typedef unsigned int GLuint;

struct VoxelTexture {

	unsigned int width, height, depth;
	float xRatio = 1, yRatio = 1, zRatio = 1;
	vector<float> voxels;
	GLuint id;

	virtual void generate();
};

struct ParametricVoxel : public VoxelTexture {

	// x, y and z are in [0;1]
	virtual float density(float x, float y, float z) = 0;

	ParametricVoxel(int size = 256) {

		width = size;
		height = size;
		depth = size;
		voxels = vector<float>(width*height*depth);
	}

	void compute() {

		for (float d = 0; d < depth; d++) {
			for (float y = 0; y < height; y++) {
				for (float x = 0; x < width; x++) {

					float v = density(x / width, y / height, d / depth);
					voxels[depth*(height*y + x) + d] = v;
				}
			}
		}
	}
};

struct VoxelSphere : public ParametricVoxel {

	float radius;

	float density(float x, float y, float z) {
		float dx = x - 0.5;
		float dy = y - 0.2;
		float dz = z - 0.5;
		return ((dx*dx + dy*dy + dz*dz) < (radius*radius)) ? 0.1 : 0.0;
	};

	VoxelSphere(int size = 256, float radius = 0.5) : ParametricVoxel(size), radius(radius) {}
};

struct VoxelCube : public VoxelTexture {

	VoxelCube() {
		width = 1;
		height = 1;
		depth = 1;
		voxels = vector<float>(1, 0.1);
	}
};

// https://en.wikipedia.org/wiki/Mandelbulb
struct VoxelMandelbulb : public ParametricVoxel {

	int order;
	int maxIter = 20;

	struct Vec3 {
		float x, y, z;
		// White and Nylander's power
		Vec3 powWN(int n) {
			switch (n) {

			case 2:

				return{
					x*x - y*y - z*z,
					2 * x*z,
					2 * x*y
				};

			case 3:

				return{
					x*x*x - 3 * x*(y*y + z*z),
					-y*y*y + 3 * y*x*x - y*z*z,
					z*z*z - 3 * z*x + z*y*y
				};

			case 4:

				return{
					x*x*x*x*x - 10 * x*x*x*(y*y + z*z) + 5 * x*(y*y*y*y + z*z*z*z),
					y*y*y*y*y - 10 * y*y*y*(z*z + x*x) + 5 * y*(z*z*z*z + x*x*x*x),
					z*z*z*z*z - 10 * z*z*z*(x*x + y*y) + 5 * z*(x*x*x*x + y*y*y*y)
				};

			default:

				if (n % 4 == 0) {
					return powWN(4).powWN(n / 4);
				}
				if (n % 3 == 0) {
					return powWN(3).powWN(n / 3);
				}
				if (n % 2 == 0) {
					return powWN(2).powWN(n / 2);
				}

				float r = sqrt(x*x + y*y + z*z);
				float phi = atan2(y, x);
				float theta = atan2(sqrt(x*x + y*y), z);
				float rn = pow(r, n);
				return{
					rn * sin(n*theta)*cos(n*phi),
					rn * sin(n*theta)*sin(n*phi),
					rn*cos(n*theta)
				};
			}
		}
		Vec3 operator+(const Vec3& off) {
			return{
				x + off.x,
				y + off.y,
				z + off.z
			};
		}
		float norm2() const {
			return x*x + y*y + z*z;
		}
		float norm() const { return sqrt(norm2()); }
	};

	float density(float x, float y, float z) {
		Vec3 coords0 = {
			2 * (x - 0.5),
			2 * (y - 0.5),
			2 * (z - 0.5)
		};
		Vec3 coords(coords0);
		int i = 0;
		while (i < maxIter) {
			if (coords.norm2()>1) { break; }
			coords = coords.powWN(order) + coords0;
			i++;
		}
		return (0.3f*i) / maxIter;
	};

	VoxelMandelbulb(int size = 256, int order = 4) : ParametricVoxel(size), order(order) {}
};

struct VoxelMRI : public VoxelTexture {

	// from MRI files (start and end are inclusive)
	VoxelMRI(const string& baseName, int start, int end) {

		depth = end - start + 1;

		for (int i = 0; i < depth; i++) {
			stringstream fileName;
			fileName << baseName << (start + i);
			string file = fileName.str();
			ifstream in(file, ios::binary | ios::ate);
			if (!in.is_open()) { cerr << "can't open " << file << endl; throw 1; }

			const int size = in.tellg();

			int w = sqrt(size / 2);

			if (i == 0) {
				width = w;
				height = w;
			}
			else if (w != width) { cerr << "Error : images from " << baseName << " have different sizes" << endl; throw 1; }

			in.seekg(0, ios::beg);
			vector<short> data(w*w);
			in.read((char*)data.data(), w*w * 2);
			for (int i = 0; i < w*w; i++) {
				voxels.push_back(_byteswap_ushort(((unsigned short*)data.data())[i]));
			}
		}

		// normalizing
		float minV = INFINITY, maxV = -INFINITY;
		for (float v : voxels) {
			if (v < minV) { minV = v; }
			if (v > maxV) { maxV = v; }
		}
		float range = (minV != maxV) ? maxV - minV : 1;
		for (int i = 0; i < voxels.size(); i++) {
			voxels[i] = (voxels[i] - minV) / range;
		}
	}
};