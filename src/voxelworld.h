#ifndef VOXELWORLD_H
#define VOXELWORLD_H

#include <Godot.hpp>
#include <SceneTree.hpp>
#include <ViewPort.hpp>
#include <Spatial.hpp>
#include <String.hpp>
#include <Array.hpp>
#include <Vector3.hpp>

#include <limits>
#include <iostream>
#include <unordered_map> 
#include <iterator>
#include <exception>
#include <stdexcept>
#include <boost/thread/mutex.hpp>

#include "constants.h"
#include "fn.h"
#include "chunkbuilder.h"
#include "meshbuilder.h"
#include "intersection.h"
#include "voxel.h"
#include "navigator.h"

using namespace std;

namespace godot {
	class GraphNavNode;

	class VoxelWorld : public Spatial {
		GODOT_CLASS(VoxelWorld, Spatial)

	private:
		std::atomic<int> width, depth;
		std::shared_ptr<VoxelWorld> self;
		std::shared_ptr<Chunk>* chunks;
		std::shared_ptr<Navigator> navigator;
		std::shared_ptr<ChunkBuilder> chunkBuilder;

		boost::mutex CHUNKS_MUTEX;
		
		void navigateTask(Vector3 startV, Vector3 goalV, int actorInstanceId);
		void buildChunksTask(std::shared_ptr<VoxelWorld> world);
	public:
		static void _register_methods();

		VoxelWorld();
		~VoxelWorld();

		void _init();

		std::shared_ptr<Chunk> intersection(int x, int y, int z);
		vector<std::shared_ptr<Chunk>> getChunksRay(Vector3 from, Vector3 to);
		std::shared_ptr<Chunk> getChunk(int x, int z);
		std::shared_ptr<Chunk> getChunk(int i);
		std::shared_ptr<Chunk> getChunk(Vector3 position);
		std::shared_ptr<GraphNavNode> getNode(Vector3 position);
		int getVoxel(Vector3 position);
		int getWidth() {
			return width;
		};
		int getDepth() {
			return depth;
		};

		void buildChunks();
		void setChunk(int x, int z, std::shared_ptr<Chunk> chunk);
		void setChunk(int i, std::shared_ptr<Chunk> chunk);
		void setVoxel(Vector3 position, float radius, bool set);
		std::shared_ptr<Navigator> getNavigator() {
			return navigator;
		};
		void setWidth(int width) {
			VoxelWorld::width = width;
		};
		void setDepth(int depth) {
			VoxelWorld::depth = depth;
		};
		void navigate(Vector3 startV, Vector3 goalV, int actorInstanceId);
	};
}

#endif