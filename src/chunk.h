#ifndef CHUNK_H
#define CHUNK_H

#include <Godot.hpp>
#include <Reference.hpp>
#include <MeshInstance.hpp>
#include <OpenSimplexNoise.hpp>

#include <vector>
#include <unordered_map>

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
		OpenSimplexNoise* noise;
		Vector3 offset;
		int amountNodes = 0;
		int amountVoxel = 0;
		int meshInstanceId = 0;
		VoxelData* volume;
		int* surfaceY;
		unordered_map<size_t, bool>* nodeChanges;
		unordered_map<size_t, Vector3>* nodes;
		bool volumeBuilt = false;
		bool building = false;
		bool assetsBuilt = false;

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
		int getVoxel(int x, int y, int z);
		int getCurrentSurfaceY(int x, int z);
		int getCurrentSurfaceY(int i);
		Ref<Voxel> getVoxelRay(Vector3 from, Vector3 to);
		unordered_map<size_t, Vector3>* getNodes() {
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
		void addNode(Vector3 p) {
			size_t hash = fn::hash(p);
			Chunk::nodes->insert(pair<size_t, Vector3>(hash, p));
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
	};

}

#endif