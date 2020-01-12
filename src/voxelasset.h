#ifndef VOXELASSET_H
#define VOXELASSET_H

#include <vector>
#include <iterator> 

#include "voxel.h"
#include "voxeldata.h"

using namespace std;

namespace godot {

	class VoxelAsset {
	private:
		int width;
		int height;
		int depth;
		int maxDeltaY;
	protected:
		VoxelData* volume;
		vector<Voxel>* voxels;
		float noiseChance;
		float noiseOffset;
		float noiseScale;
	public:
		VoxelAsset() : VoxelAsset(0, 0, 0, 0) {};
		VoxelAsset(int w, int h, int d, int md) {
			VoxelAsset::width = w;
			VoxelAsset::height = h;
			VoxelAsset::depth = d;
			VoxelAsset::maxDeltaY = md;
			VoxelAsset::volume = new VoxelData(w, h, d);
			VoxelAsset::voxels = new vector<Voxel>();
			VoxelAsset::noiseChance = 1.0;
			VoxelAsset::noiseOffset = 1.0;
			VoxelAsset::noiseScale = 1.0;
		};
		void setVoxel(int x, int y, int z, int v) {
			volume->set(x, y, z, v);
			voxels->push_back(Voxel(Vector3(x, y, z), v));
		};
		vector<Voxel>* getVoxels() {
			return voxels;
		};
		VoxelData* getVolume() {
			return volume;
		};
		float getNoiseChance() { return VoxelAsset::noiseChance; };
		float getNoiseOffset() { return VoxelAsset::noiseOffset; };
		float getNoiseScale() { return VoxelAsset::noiseScale; };
		int getWidth() { return VoxelAsset::width; };
		int getHeight() { return VoxelAsset::height; };
		int getDepth() { return VoxelAsset::depth; };
		int getMaxDeltaY() { return VoxelAsset::maxDeltaY; };
	};
}

#endif