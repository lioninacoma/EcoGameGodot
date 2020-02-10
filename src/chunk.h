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
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_types.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/exception/diagnostic_information.hpp> 
#include <boost/exception_ptr.hpp> 

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

		boost::shared_ptr<VoxelData> volume;
		unordered_map<size_t, boost::shared_ptr<GraphNode>>* nodes;

		int* surfaceY; // TODO: provide thread safety?
		
		OpenSimplexNoise* noise;

		boost::shared_timed_mutex CHUNK_NODES_MUTEX;
		boost::shared_timed_mutex OFFSET_MUTEX;

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
			boost::shared_lock<boost::shared_timed_mutex> lock(OFFSET_MUTEX);
			return offset;
		}
		boost::shared_ptr<VoxelData> getVolume() {
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
		void forEachNode(std::function<void(std::pair<size_t, boost::shared_ptr<GraphNode>>)> func) {
			boost::shared_lock<boost::shared_timed_mutex> lock(CHUNK_NODES_MUTEX);
			std::for_each(nodes->begin(), nodes->end(), func);
		};
		boost::shared_ptr<GraphNode> getNode(size_t hash) {
			boost::shared_lock<boost::shared_timed_mutex> lock(CHUNK_NODES_MUTEX);
			auto it = Chunk::nodes->find(hash);
			if (it == Chunk::nodes->end()) return NULL;
			return it->second;
		};
		boost::shared_ptr<GraphNode> findNode(Vector3 position) {
			boost::shared_lock<boost::shared_timed_mutex> lock(CHUNK_NODES_MUTEX);
			boost::shared_ptr<GraphNode> closest;
			float minDist = numeric_limits<float>::max();
			float dist;
			for (auto node : *nodes) {
				dist = node.second->getPoint()->distance_to(position);
				if (dist < minDist) {
					minDist = dist;
					closest = node.second;
				}
			}
			return closest;
		};
		PoolVector3Array getReachableVoxelsOfType(Vector3 point, int type) {
			PoolVector3Array voxels;
			Vector3 chunkOffset;
			int x, y, z, nx, ny, nz, v;
			for (z = -1; z < 2; z++)
				for (x = -1; x < 2; x++)
					for (y = -1; y < 2; y++) {
						nx = point.x + x;
						ny = point.y + y;
						nz = point.z + z;
						//Godot::print(String("point: {0}").format(Array::make(point)));

						chunkOffset.x = nx;
						chunkOffset.y = ny;
						chunkOffset.z = nz;
						chunkOffset = fn::toChunkCoords(chunkOffset);
						chunkOffset *= Vector3(CHUNK_SIZE_X, 0, CHUNK_SIZE_Z);

						if (offset != chunkOffset) {
							continue;
						}

						v = getVoxel(
							nx % CHUNK_SIZE_X, 
							ny % CHUNK_SIZE_Y,
							nz % CHUNK_SIZE_Z);

						if (v != type) continue;

						voxels.push_back(Vector3(nx, ny, nz));
					}
			return voxels;
		};
		// setter
		void setOffset(Vector3 offset) {
			boost::unique_lock<boost::shared_timed_mutex> lock(OFFSET_MUTEX);
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

		void addNode(boost::shared_ptr<GraphNode> node) {
			boost::unique_lock<boost::shared_timed_mutex> lock(CHUNK_NODES_MUTEX);
			try {
				size_t hash = node->getHash();
				Chunk::nodes->insert(pair<size_t, boost::shared_ptr<GraphNode>>(hash, node));
			}
			catch (const std::exception & e) {
				std::cerr << boost::diagnostic_information(e);
			}
		};
		void removeNode(boost::shared_ptr<GraphNode> node) {
			boost::unique_lock<boost::shared_timed_mutex> lock(CHUNK_NODES_MUTEX);
			try {
				size_t hash = node->getHash();
				if (Chunk::nodes->find(hash) == Chunk::nodes->end()) return;
				Chunk::nodes->erase(hash);
			}
			catch (const std::exception & e) {
				std::cerr << boost::diagnostic_information(e);
			}
		};
	};

}

#endif