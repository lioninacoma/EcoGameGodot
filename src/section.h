#ifndef SECTION_H
#define SECTION_H

#include <Godot.hpp>
#include <Vector3.hpp>
#include <Texture.hpp>
#include <OpenSimplexNoise.hpp>

#include <iostream>
#include <string>
#include <exception>
#include <stdexcept>
#include <atomic>

#include <boost/thread/mutex.hpp>
#include <boost/exception/diagnostic_information.hpp> 
#include <boost/exception_ptr.hpp>

#include "constants.h"
#include "chunkbuilder.h"
#include "fn.h"
#include "chunk.h"
#include "area.h"
#include "voxel.h"

using namespace std;

namespace godot {

	class GraphNavNode;
	class VoxelWorld;

	class Section {

	private:
		std::shared_ptr<VoxelWorld> world;
		std::shared_ptr<Chunk>* chunks;
		std::atomic<int> sectionSize;
		std::atomic<int> sectionChunksLen;
		OpenSimplexNoise* noise;

		Vector2 offset;

		boost::mutex CHUNKS_MUTEX;
		std::shared_ptr<Chunk> intersection(int x, int y, int z);
	public:
		Section(std::shared_ptr<VoxelWorld> world) : Section(world, Vector2()) {};
		Section(std::shared_ptr<VoxelWorld> world, Vector2 offset) : Section(world, offset, SECTION_SIZE) {};
		Section(std::shared_ptr<VoxelWorld> world, Vector2 offset, int sectionSize);

		~Section();

		vector<std::shared_ptr<Chunk>> getChunksRay(Vector3 from, Vector3 to);
		Array getDisconnectedVoxels(Vector3 position, Vector3 start, Vector3 end);
		PoolVector3Array findVoxelsInRange(Vector3 startV, float radius, int voxel);
		int getVoxel(Vector3 position);
		std::shared_ptr<GraphNavNode> getNode(Vector3 position);
		std::shared_ptr<Chunk> getChunk(int x, int z);
		std::shared_ptr<Chunk> getChunk(int i);

		void build(std::shared_ptr<ChunkBuilder> builder);
		void setChunk(int x, int z, std::shared_ptr<Chunk> chunk);
		void setChunk(int i, std::shared_ptr<Chunk> chunk);
		void fill(int sectionSize);

		Vector2 getOffset();
	};
}

#endif