#ifndef VOXELASSET_PINETREE_H
#define VOXELASSET_PINETREE_H

#include "voxelasset.h"
#include "fn.h"

using namespace std;

namespace godot {

	class VoxelAsset_PineTree : public VoxelAsset {
	public:
		VoxelAsset_PineTree() : VoxelAsset(5, 6, 5, 2) {
			const int w = getWidth();
			const int h = getHeight();
			const int d = getDepth();
			int y;

			// trunk
			setVoxel(2, 0, 2, 4);
			setVoxel(2, 1, 2, 4);
			setVoxel(2, 2, 2, 4);

			int zd = 0;
			int xw = 0;

			// leaves
			for (int y = 3; y < h; y++) {
				for (int z = zd; z < d - zd; z++) {
					for (int x = xw; x < w - xw; x++) {
						setVoxel(x, y, z, 5);
					}
				}
				zd++;
				xw++;
			}
		};
	};
}

#endif