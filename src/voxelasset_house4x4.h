#ifndef VOXELASSET_HOUSE4X4_H
#define VOXELASSET_HOUSE4X4_H

#include "voxelasset.h"
#include "fn.h"

using namespace std;

namespace godot {

	class VoxelAsset_House4x4 : public VoxelAsset {
	public:
		VoxelAsset_House4x4() : VoxelAsset(4, 4, 4, 0) {};
		vector<Voxel>* getVoxels() {
			const int w = getWidth();
			const int h = getHeight();
			const int z = getDepth();
			int y;

			auto voxels = new vector<Voxel>();

			// walls
			for (y = 0; y < h - 1; y++) {
				voxels->push_back(Voxel(Vector3(0, y, 0), 2));
				voxels->push_back(Voxel(Vector3(1, y, 0), 2));
				voxels->push_back(Voxel(Vector3(2, y, 0), 2));
				voxels->push_back(Voxel(Vector3(3, y, 0), 2));
				voxels->push_back(Voxel(Vector3(3, y, 1), 2));
				voxels->push_back(Voxel(Vector3(3, y, 2), 2));
				voxels->push_back(Voxel(Vector3(3, y, 3), 2));
				voxels->push_back(Voxel(Vector3(1, y, 3), 2));
				voxels->push_back(Voxel(Vector3(0, y, 3), 2));
				voxels->push_back(Voxel(Vector3(0, y, 2), 2));
				voxels->push_back(Voxel(Vector3(0, y, 1), 2));

				voxels->push_back(Voxel(Vector3(1, y, 1), 0));
				voxels->push_back(Voxel(Vector3(1, y, 2), 0));
				voxels->push_back(Voxel(Vector3(2, y, 1), 0));
				voxels->push_back(Voxel(Vector3(2, y, 2), 0));
			}

			// ceiling
			for (y = 2; y < h - 1; y++) {
				voxels->push_back(Voxel(Vector3(1, y, 1), 2));
				voxels->push_back(Voxel(Vector3(1, y, 2), 2));
				voxels->push_back(Voxel(Vector3(2, y, 1), 2));
				voxels->push_back(Voxel(Vector3(2, y, 2), 2));
			}

			// door
			//voxels->push_back(Voxel(Vector3(2, 0, 4), 0));
			voxels->push_back(Voxel(Vector3(2, 0, 3), 0));
			voxels->push_back(Voxel(Vector3(2, 1, 3), 0));
			voxels->push_back(Voxel(Vector3(2, 2, 3), 2));

			return voxels;
		};
	};
}

#endif