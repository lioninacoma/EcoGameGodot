#ifndef VOXELASSETMANAGER_H
#define VOXELASSETMANAGER_H

#include <map>
#include <vector>
#include <iterator> 

#include "voxel.h"
#include "voxelasset_house4x4.h"
#include "voxelasset_house6x6.h"
#include "voxelasset_pinetree.h"

using namespace std;

namespace godot {

	enum class VoxelAssetType {
		HOUSE_4X4,
		HOUSE_6X6,
		PINE_TREE
	};

	class VoxelAssetManager {
	private:
		std::map<VoxelAssetType, VoxelAsset*> voxels;

		VoxelAssetManager() {
			voxels.insert(std::pair<VoxelAssetType, VoxelAsset*>(VoxelAssetType::HOUSE_4X4, new VoxelAsset_House4x4()));
			voxels.insert(std::pair<VoxelAssetType, VoxelAsset*>(VoxelAssetType::HOUSE_6X6, new VoxelAsset_House6x6()));
			voxels.insert(std::pair<VoxelAssetType, VoxelAsset*>(VoxelAssetType::PINE_TREE, new VoxelAsset_PineTree()));
		}
	public:
		static VoxelAssetManager* get() {
			static auto manager = new VoxelAssetManager();
			return manager;
		}

		VoxelAsset* getVoxelAsset(VoxelAssetType type) {
			auto pos = voxels.find(type);

			if (pos == voxels.end()) {
				return NULL;
			}
			else {
				return pos->second;
			}
		}

		vector<Voxel>* getVoxels(VoxelAssetType type) {
			auto pos = voxels.find(type);

			if (pos == voxels.end()) {
				return NULL;
			}
			else {
				return pos->second->getVoxels();
			}
		}
	};
}

#endif