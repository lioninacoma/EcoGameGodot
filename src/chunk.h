#ifndef CHUNK_H
#define CHUNK_H

#include <Godot.hpp>
#include <Reference.hpp>
#include <MeshInstance.hpp>
#include <OpenSimplexNoise.hpp>

#include <vector>
#include <unordered_map>
#include <limits>

#include <boost/atomic.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_types.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>

#include "constants.h"
#include "fn.h"
#include "intersection.h"
#include "voxel.h"
#include "voxeldata.h"
#include "graphnode.h"

using namespace std;

namespace godot {

	class Chunk : public Reference {
		GODOT_CLASS(Chunk, Reference)
	private:
		boost::atomic<int> amountNodes = 0;
		boost::atomic<int> amountVoxel = 0;
		boost::atomic<int> meshInstanceId = 0;
		boost::atomic<bool> navigatable = false;
		boost::atomic<bool> building = false;
		boost::atomic<bool> volumeBuilt = false;

		Vector3 offset;

		VoxelData* volume;
		unordered_map<size_t, bool>* nodeChanges;
		unordered_map<size_t, GraphNode*>* nodes;

		int* surfaceY; // TODO: provide thread safety?
		
		OpenSimplexNoise* noise;

		boost::shared_timed_mutex CHUNK_NODES_MUTEX;

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
		bool isNavigatable() {
			return navigatable;
		};
		bool isBuilding() {
			return building;
		};
		int getVoxel(int x, int y, int z);
		int getCurrentSurfaceY(int x, int z);
		int getCurrentSurfaceY(int i);
		Ref<Voxel> getVoxelRay(Vector3 from, Vector3 to);
		unordered_map<size_t, GraphNode*>* getNodes() {
			return Chunk::nodes;
		};
		void lockNodes() {
			CHUNK_NODES_MUTEX.lock_shared();
		};
		void unlockNodes() {
			CHUNK_NODES_MUTEX.unlock_shared();
		};
		unordered_map<size_t, bool>* getNodeChanges() {
			return Chunk::nodeChanges;
		};
		GraphNode* getNode(size_t hash) {
			boost::shared_lock<boost::shared_timed_mutex> lock(CHUNK_NODES_MUTEX);
			auto it = Chunk::nodes->find(hash);
			if (it == Chunk::nodes->end()) return NULL;
			return it->second;
		};
		// setter
		void setOffset(Vector3 offset) {
			Chunk::offset = offset;
		};
		void setNavigatable() {
			navigatable = true;
		};
		void setBuilding(bool building) {
			Chunk::building = building;
		};
		void setMeshInstanceId(int meshInstanceId) {
			Chunk::meshInstanceId = meshInstanceId;
		};
		void setVoxel(int x, int y, int z, int v);
		int buildVolume();
		void clearNodeChanges() {
			boost::unique_lock<boost::shared_timed_mutex> lock(CHUNK_NODES_MUTEX);
			nodeChanges->clear();
		}
		void addNode(GraphNode* node) {
			boost::unique_lock<boost::shared_timed_mutex> lock(CHUNK_NODES_MUTEX);
			size_t hash = node->getHash();
			Chunk::nodes->insert(pair<size_t, GraphNode*>(hash, node));
			//Chunk::nodeChanges->emplace(hash, 1);

			auto it = Chunk::nodeChanges->find(hash);
			if (it == Chunk::nodeChanges->end()) {
				Chunk::nodeChanges->emplace(hash, 1);
			}
			else if (!it->second) {
				Chunk::nodeChanges->erase(hash);
			}
		};
		void removeNode(Vector3 point) {
			boost::unique_lock<boost::shared_timed_mutex> lock(CHUNK_NODES_MUTEX);
			size_t hash = fn::hash(point);
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