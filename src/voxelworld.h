#ifndef VOXELWORLD_H
#define VOXELWORLD_H

#include <Godot.hpp>
#include <SceneTree.hpp>
#include <ViewPort.hpp>
#include <Spatial.hpp>
#include <String.hpp>
#include <Array.hpp>
#include <Vector3.hpp>
#include <FuncRef.hpp>

#include <limits>
#include <vector>
#include <iostream>
#include <unordered_map> 
#include <iterator>
#include <exception>
#include <stdexcept>

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>

#include "constants.h"
#include "fn.h"
#include "quadtreebuilder.h"
#include "chunkbuilder.h"
#include "meshbuilder.h"
#include "intersection.h"
#include "voxel.h"
#include "navigator.h"

using namespace std;

namespace godot {
	class GraphNode;

	class VoxelWorld : public Spatial {
		GODOT_CLASS(VoxelWorld, Spatial)

	private:
		std::atomic<int> size;
		std::shared_ptr<VoxelWorld> self;
		//unordered_map<size_t, std::shared_ptr<Chunk>> chunks;
		vector<std::shared_ptr<Chunk>> chunks;
		vector<quadsquare*> quadtrees;
		std::unique_ptr<Navigator> navigator;
		std::unique_ptr<ChunkBuilder> chunkBuilder;
		std::unique_ptr<QuadTreeBuilder> quadtreeBuilder;
		Ref<FuncRef> isVoxelFn;

		std::shared_mutex CHUNKS_MUTEX;

		void navigateTask(Vector3 startV, Vector3 goalV, int actorInstanceId);
		void buildChunksTask();
		void buildChunksTaskSphere(Vector3 cameraPosition, float radius);
		void buildQuadTreesTask(Vector3 cameraPosition);
		void prepareChunkTask(std::shared_ptr<Chunk> chunk, float lod);
	public:
		static void _register_methods();

		VoxelWorld();
		~VoxelWorld();

		void _init();
		void _notification(const int64_t what);

		std::shared_ptr<Chunk> intersection(int x, int y, int z);
		vector<std::shared_ptr<Chunk>> getChunksRay(Vector3 from, Vector3 to);
		std::shared_ptr<Chunk> getChunk(int i);
		std::shared_ptr<Chunk> getChunk(int x, int y, int z);
		std::shared_ptr<Chunk> getChunk(Vector3 position);
		std::shared_ptr<GraphNode> getNode(Vector3 position);
		std::shared_ptr<GraphNode> findClosestNode(Vector3 position);
		int getSize();

		void buildChunks(Vector3 cameraPosition, float radius);
		void buildQuadTrees(Vector3 cameraPosition);
		void setIsVoxelFn(Variant fnRef);
		void setIsWalkableFn(Variant fnRef);
		void setChunk(int x, int y, int z, std::shared_ptr<Chunk> chunk);
		void setVoxel(Vector3 position, float radius, bool set);
		void setSize(float size);
		void navigate(Vector3 startV, Vector3 goalV, int actorInstanceId);
	};
}

#endif