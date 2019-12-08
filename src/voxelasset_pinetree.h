#ifndef VOXELASSET_PINETREE_H
#define VOXELASSET_PINETREE_H

#include "voxelasset.h"
#include "fn.h"

using namespace std;

namespace godot {

	class VoxelAsset_PineTree : public VoxelAsset {
	public:
		VoxelAsset_PineTree() : VoxelAsset(5, 5, 5, 2) {};
		vector<Voxel>* getVoxels() {
			const int w = getWidth();
			const int h = getHeight();
			const int d = getDepth();
			int y;

			auto tree = new vector<Voxel>();

			// trunk
			tree->push_back(Voxel(Vector3(2, 0, 2), 2));
			tree->push_back(Voxel(Vector3(2, 1, 2), 2));

			int zd = 0;
			int xw = 0;

			// leaves
			for (int y = 2; y < h; y++) {
				for (int z = zd; z < d - zd; z++) {
					for (int x = xw; x < w - xw; x++) {
						tree->push_back(Voxel(Vector3(x, y, z), 1));
					}
				}
				zd++;
				xw++;
			}

			return tree;
		};
	};
}

#endif