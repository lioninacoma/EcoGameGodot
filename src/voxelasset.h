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
		int maxDeltaY;
	public:
		VoxelAsset() : VoxelAsset(0, 0, 0, 0) {};
		VoxelAsset(int w, int h, int d, int md) {
			VoxelAsset::width = w;
			VoxelAsset::height = h;
			VoxelAsset::depth = d;
			VoxelAsset::maxDeltaY = md;
		};
		virtual vector<Voxel>* getVoxels() {
			return new vector<Voxel>();
		};
		int getWidth() { return VoxelAsset::width; };
		int getHeight() { return VoxelAsset::height; };
		int getDepth() { return VoxelAsset::depth; };
		int getMaxDeltaY() { return VoxelAsset::maxDeltaY; };
	};
}

#endif