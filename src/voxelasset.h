#ifndef VOXELASSET_H
#define VOXELASSET_H

#include <vector>
#include <iterator> 

#include "voxel.h"

using namespace std;

namespace godot {

	class VoxelAsset {
	private:
		int width;
		int height;
		int depth;
	public:
		VoxelAsset() : VoxelAsset(0, 0, 0) {};
		VoxelAsset(int w, int h, int d) {
			VoxelAsset::width = w;
			VoxelAsset::height = h;
			VoxelAsset::depth = d;
		};
		virtual vector<Voxel> getVoxels() {
			return vector<Voxel>();
		};
		int getWidth() { return VoxelAsset::width; };
		int getHeight() { return VoxelAsset::height; };
		int getDepth() { return VoxelAsset::depth; };
	};
}

#endif