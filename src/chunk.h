#ifndef CHUNK_H
#define CHUNK_H

#include <Godot.hpp>
#include <Reference.hpp>
#include <MeshInstance.hpp>
#include <OpenSimplexNoise.hpp>

#include <vector>
#include <unordered_map>
#include <limits>

#include <boost/function.hpp>
#include <boost/bind.hpp>

#include "constants.h"
#include "fn.h"
#include "intersection.h"
#include "voxel.h"
#include "voxeldata.h"

using namespace std;

namespace godot {

	class Chunk : public Reference {
		GODOT_CLASS(Chunk, Reference)
	private:
		OpenSimplexNoise* noise,* noiseP;
		Vector3 offset;
		int amountNodes = 0;
		int amountVoxel = 0;
		int meshInstanceId = 0;
		VoxelData* volume;
		int* surfaceY;
		unordered_map<size_t, bool>* nodeChanges;
		unordered_map<size_t, Voxel>* nodes;
		bool volumeBuilt = false;
		bool building = false;
		bool assetsBuilt = false;
		bool navigatable = false;

		int getVoxelY(int x, int z);
		float getVoxelChance(int x, int y, int z);
		Voxel* intersection(int x, int y, int z);
	public:
		static void _register_methods();

		Chunk() : Chunk(Vector3(0, 0, 0)) {};
		Chunk(Vector3 offset);
		~Chunk();

		void _init(); // our initializer called by Godot
		
		// getter
		Vector3 getOffset() {
			return offset;
		}
		VoxelData* getVolume() {
			return volume;
		}
		int getMeshInstanceId() {
			return meshInstanceId;
		};
		bool isBuilding() {
			return Chunk::building;
		}
		bool isAssetsBuilt() {
			return Chunk::assetsBuilt;
		}
		bool isReady() {
			return Chunk::volumeBuilt && !Chunk::building && Chunk::meshInstanceId > 0;
		}
		bool doUpdateGraph() {
			return navigatable && !nodeChanges->empty();
		}
		int getVoxel(int x, int y, int z);
		int getCurrentSurfaceY(int x, int z);
		int getCurrentSurfaceY(int i);
		Ref<Voxel> getVoxelRay(Vector3 from, Vector3 to);
		unordered_map<size_t, Voxel>* getNodes() {
			return Chunk::nodes;
		};
		int getAmountNodes() {
			return Chunk::nodes->size();
		};
		unordered_map<size_t, bool>* getNodeChanges() {
			return nodeChanges;
		};

		// setter
		void setOffset(Vector3 offset) {
			Chunk::offset = offset;
		};
		void setBuilding(bool building) {
			Chunk::building = building;
		};
		void markAssetsBuilt() {
			Chunk::assetsBuilt = true;
		};
		void setMeshInstanceId(int meshInstanceId) {
			Chunk::meshInstanceId = meshInstanceId;
		};
		void setVoxel(int x, int y, int z, int v);
		int buildVolume();
		void addNode(Voxel p) {
			size_t hash = fn::hash(p.getPosition());
			Chunk::nodes->insert(pair<size_t, Voxel>(hash, p));
			//Chunk::nodeChanges->emplace(hash, 1);

			auto it = Chunk::nodeChanges->find(hash);
			if (it == Chunk::nodeChanges->end()) {
				Chunk::nodeChanges->emplace(hash, 1);
			}
			else if (!it->second) {
				Chunk::nodeChanges->erase(hash);
			}
		};
		void removeNode(Vector3 p) {
			size_t hash = fn::hash(p);
			if (Chunk::nodes->find(hash) == Chunk::nodes->end()) return;
			Chunk::nodes->erase(hash);
			//Chunk::nodeChanges->emplace(hash, 0);

			auto it = Chunk::nodeChanges->find(hash);
			if (it == Chunk::nodeChanges->end()) {
				Chunk::nodeChanges->emplace(hash, 0);
			}
			else if (it->second) {
				Chunk::nodeChanges->erase(hash);
			}
		};
		PoolVector3Array findVoxels(int type) {
			PoolVector3Array voxels;
			for (auto& current : *nodes) {
				if ((int)current.second.getType() == type) {
					voxels.push_back(current.second.getPosition());
				}
			}
			return voxels;
		};
		Vector3* findClosestVoxel(Vector3 pos, int type) {
			Vector3* closest = NULL;
			float minDistance = numeric_limits<float>::max();
			float distance;

			for (auto& current : *nodes) {
				distance = current.second.getPosition().distance_to(pos);
				if ((int)current.second.getType() == type && distance < minDistance) {
					closest = &current.second.getPosition();
					minDistance = distance;
				}
			}
			return closest;
		};
		void setNavigatable() {
			navigatable = true;
		}
	};

}

#endif