#ifndef VOXELASSET_HOUSE6X6_H
#define VOXELASSET_HOUSE6X6_H

#include "voxelasset.h"
#include "fn.h"

using namespace std;

namespace godot {

	class VoxelAsset_House6x6 : public VoxelAsset {
	public:
		VoxelAsset_House6x6() : VoxelAsset(6, 6, 6, 0) {
			const int w = getWidth();
			const int h = getHeight();
			const int z = getDepth();
			int y;

			// walls
			for (y = 0; y < h - 2; y++) {
				setVoxel(0, y, 0, 2);
				setVoxel(1, y, 0, 2);
				setVoxel(2, y, 0, 2);
				setVoxel(3, y, 0, 2);
				setVoxel(4, y, 0, 2);
				setVoxel(5, y, 0, 2);
				setVoxel(5, y, 1, 2);
				setVoxel(5, y, 2, 2);
				setVoxel(5, y, 3, 2);
				setVoxel(5, y, 4, 2);
				setVoxel(5, y, 5, 2);
				setVoxel(4, y, 5, 2);
				setVoxel(2, y, 5, 2);
				setVoxel(1, y, 5, 2);
				setVoxel(0, y, 5, 2);
				setVoxel(0, y, 4, 2);
				setVoxel(0, y, 3, 2);
				setVoxel(0, y, 2, 2);
				setVoxel(0, y, 1, 2);

				setVoxel(1, y, 1, 0);
				setVoxel(2, y, 1, 0);
				setVoxel(3, y, 1, 0);
				setVoxel(4, y, 1, 0);
				setVoxel(1, y, 2, 0);
				setVoxel(4, y, 2, 0);
				setVoxel(1, y, 3, 0);
				setVoxel(4, y, 3, 0);
				setVoxel(1, y, 4, 0);
				setVoxel(2, y, 4, 0);
				setVoxel(3, y, 4, 0);
				setVoxel(4, y, 4, 0);
				setVoxel(2, y, 2, 0);
				setVoxel(2, y, 3, 0);
				setVoxel(3, y, 2, 0);
				setVoxel(3, y, 3, 0);
			}

			// ceiling
			for (y = 4; y < h - 1; y++) {
				setVoxel(1, y, 1, 2);
				setVoxel(2, y, 1, 2);
				setVoxel(3, y, 1, 2);
				setVoxel(4, y, 1, 2);
				setVoxel(1, y, 2, 2);
				setVoxel(4, y, 2, 2);
				setVoxel(1, y, 3, 2);
				setVoxel(4, y, 3, 2);
				setVoxel(1, y, 4, 2);
				setVoxel(2, y, 4, 2);
				setVoxel(3, y, 4, 2);
				setVoxel(4, y, 4, 2);
			}

			for (y = 4; y < h; y++) {
				setVoxel(2, y, 2, 2);
				setVoxel(2, y, 3, 2);
				setVoxel(3, y, 2, 2);
				setVoxel(3, y, 3, 2);
			}

			// door
			setVoxel(3, 0, 5, 0);
			setVoxel(3, 1, 5, 0);
			setVoxel(3, 2, 5, 2);
			setVoxel(3, 3, 5, 2);
		};
	};
}

#endif