#ifndef VOXELASSET_HOUSE6X6_H
#define VOXELASSET_HOUSE6X6_H

#include "voxelasset.h"
#include "fn.h"

using namespace std;

namespace godot {

	class VoxelAsset_House6x6 : public VoxelAsset {
	public:
		VoxelAsset_House6x6() : VoxelAsset(6, 6, 6, 0) {};
		vector<Voxel>* getVoxels() {
			const int w = getWidth();
			const int h = getHeight();
			const int z = getDepth();
			int y;

			auto voxels = new vector<Voxel>();

			// walls
			for (y = 0; y < h - 2; y++) {
				voxels->push_back(Voxel(Vector3(0, y, 0), 2));
				voxels->push_back(Voxel(Vector3(1, y, 0), 2));
				voxels->push_back(Voxel(Vector3(2, y, 0), 2));
				voxels->push_back(Voxel(Vector3(3, y, 0), 2));
				voxels->push_back(Voxel(Vector3(4, y, 0), 2));
				voxels->push_back(Voxel(Vector3(5, y, 0), 2));
				voxels->push_back(Voxel(Vector3(5, y, 1), 2));
				voxels->push_back(Voxel(Vector3(5, y, 2), 2));
				voxels->push_back(Voxel(Vector3(5, y, 3), 2));
				voxels->push_back(Voxel(Vector3(5, y, 4), 2));
				voxels->push_back(Voxel(Vector3(5, y, 5), 2));
				voxels->push_back(Voxel(Vector3(4, y, 5), 2));
				voxels->push_back(Voxel(Vector3(2, y, 5), 2));
				voxels->push_back(Voxel(Vector3(1, y, 5), 2));
				voxels->push_back(Voxel(Vector3(0, y, 5), 2));
				voxels->push_back(Voxel(Vector3(0, y, 4), 2));
				voxels->push_back(Voxel(Vector3(0, y, 3), 2));
				voxels->push_back(Voxel(Vector3(0, y, 2), 2));
				voxels->push_back(Voxel(Vector3(0, y, 1), 2));

				voxels->push_back(Voxel(Vector3(1, y, 1), 0));
				voxels->push_back(Voxel(Vector3(2, y, 1), 0));
				voxels->push_back(Voxel(Vector3(3, y, 1), 0));
				voxels->push_back(Voxel(Vector3(4, y, 1), 0));
				voxels->push_back(Voxel(Vector3(1, y, 2), 0));
				voxels->push_back(Voxel(Vector3(4, y, 2), 0));
				voxels->push_back(Voxel(Vector3(1, y, 3), 0));
				voxels->push_back(Voxel(Vector3(4, y, 3), 0));
				voxels->push_back(Voxel(Vector3(1, y, 4), 0));
				voxels->push_back(Voxel(Vector3(2, y, 4), 0));
				voxels->push_back(Voxel(Vector3(3, y, 4), 0));
				voxels->push_back(Voxel(Vector3(4, y, 4), 0));
				voxels->push_back(Voxel(Vector3(2, y, 2), 0));
				voxels->push_back(Voxel(Vector3(2, y, 3), 0));
				voxels->push_back(Voxel(Vector3(3, y, 2), 0));
				voxels->push_back(Voxel(Vector3(3, y, 3), 0));
			}

			// ceiling
			for (y = 4; y < h - 1; y++) {
				voxels->push_back(Voxel(Vector3(1, y, 1), 2));
				voxels->push_back(Voxel(Vector3(2, y, 1), 2));
				voxels->push_back(Voxel(Vector3(3, y, 1), 2));
				voxels->push_back(Voxel(Vector3(4, y, 1), 2));
				voxels->push_back(Voxel(Vector3(1, y, 2), 2));
				voxels->push_back(Voxel(Vector3(4, y, 2), 2));
				voxels->push_back(Voxel(Vector3(1, y, 3), 2));
				voxels->push_back(Voxel(Vector3(4, y, 3), 2));
				voxels->push_back(Voxel(Vector3(1, y, 4), 2));
				voxels->push_back(Voxel(Vector3(2, y, 4), 2));
				voxels->push_back(Voxel(Vector3(3, y, 4), 2));
				voxels->push_back(Voxel(Vector3(4, y, 4), 2));
			}

			for (y = 4; y < h; y++) {
				voxels->push_back(Voxel(Vector3(2, y, 2), 2));
				voxels->push_back(Voxel(Vector3(2, y, 3), 2));
				voxels->push_back(Voxel(Vector3(3, y, 2), 2));
				voxels->push_back(Voxel(Vector3(3, y, 3), 2));
			}

			// door
			//voxels->push_back(Voxel(Vector3(3, 0, 6), 0));
			voxels->push_back(Voxel(Vector3(3, 0, 5), 0));
			voxels->push_back(Voxel(Vector3(3, 1, 5), 0));
			voxels->push_back(Voxel(Vector3(3, 2, 5), 2));
			voxels->push_back(Voxel(Vector3(3, 3, 5), 2));
			//voxels->push_back(Voxel(Vector3(3, 4, 5), 2));

			return voxels;
		};
	};
}

#endif