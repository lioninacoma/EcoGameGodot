#ifndef VOXELASSETMANAGER_H
#define VOXELASSETMANAGER_H

#include <map>
#include <vector>
#include <iterator> 

#include "voxel.h"
#include "voxeldata.h"
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
		std::map<VoxelAssetType, VoxelAsset*> assets;

		VoxelAssetManager() {
			assets.insert(std::pair<VoxelAssetType, VoxelAsset*>(VoxelAssetType::HOUSE_4X4, new VoxelAsset_House4x4()));
			assets.insert(std::pair<VoxelAssetType, VoxelAsset*>(VoxelAssetType::HOUSE_6X6, new VoxelAsset_House6x6()));
			assets.insert(std::pair<VoxelAssetType, VoxelAsset*>(VoxelAssetType::PINE_TREE, new VoxelAsset_PineTree()));
		}
	public:
		static VoxelAssetManager* get() {
			static auto manager = new VoxelAssetManager();
			return manager;
		}

		VoxelAsset* getVoxelAsset(VoxelAssetType type) {
			return assets[type];
		}

		vector<Voxel>* getVoxels(VoxelAssetType type) {
			return assets[type]->getVoxels();
		}

		std::shared_ptr<VoxelData> getVolume(VoxelAssetType type) {
			return assets[type]->getVolume();
		}
	};
}

#endif