#ifndef VOXELDATA_H
#define VOXELDATA_H

#include <Godot.hpp>

#include "constants.h"
#include "fn.h"

using namespace std;

namespace godot {
	class VoxelData {
	private:
		char* volume;
		int width, height, depth;
	public:
		VoxelData(int width, int height, int depth) {
			VoxelData::width = width;
			VoxelData::height = height;
			VoxelData::depth = depth;
			VoxelData::volume = new char[width * height * depth];
			memset(volume, 0, width * height * depth * sizeof(*volume));
		};
		~VoxelData() {
			delete[] volume;
		};
		int get(int x, int y, int z) {
			return volume[fn::fi3(x, y, z, width, height)];
		};
		void set(int x, int y, int z, int v) {
			volume[fn::fi3(x, y, z, width, height)] = v;
		};
		int getWidth() {
			return width;
		};
		int getHeight() {
			return height;
		};
		int getDepth() {
			return depth;
		};
	};
}

#endif