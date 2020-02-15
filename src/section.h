#ifndef SECTION_H
#define SECTION_H

#include <Godot.hpp>
#include <Node.hpp>
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

#include "voxelassetmanager.h"
#include "constants.h"
#include "chunkbuilder.h"
#include "fn.h"
#include "chunk.h"
#include "area.h"
#include "graphnode.h"
#include "voxel.h"

using namespace std;

namespace godot {

	class EcoGame;

	class Section : public Node {
		GODOT_CLASS(Section, Node)

	private:
		std::shared_ptr<Chunk>* chunks;
		std::atomic<int> sectionSize;
		std::atomic<int> sectionChunksLen;
		OpenSimplexNoise* noise;

		Vector2 offset;

		boost::mutex CHUNKS_MUTEX;
		std::shared_ptr<Chunk> intersection(int x, int y, int z);
	public:
		static ObjectPool<int, INT_POOL_BUFFER_SIZE, 4>& getIntBufferPool() {
			static ObjectPool<int, INT_POOL_BUFFER_SIZE, 4> pool;
			return pool;
		};

		static void _register_methods();

		Section() : Section(Vector2()) {};
		Section(Vector2 offset) : Section(offset, SECTION_SIZE) {};
		Section(Vector2 offset, int sectionSize);

		~Section();

		// our initializer called by Godot
		void _init() {

		}

		vector<std::shared_ptr<Chunk>> getChunksRay(Vector3 from, Vector3 to);
		float getVoxelAssetChance(int x, int y, float scale);
		void addVoxelAsset(Vector3 startV, VoxelAssetType type, std::shared_ptr<ChunkBuilder> builder, Node* game);
		Array getDisconnectedVoxels(Vector3 position, Vector3 start, Vector3 end);
		PoolVector3Array findVoxelsInRange(Vector3 startV, float radius, int voxel);
		bool voxelAssetFits(Vector3 start, VoxelAssetType type);
		void setVoxel(Vector3 position, int voxel, std::shared_ptr<ChunkBuilder> builder, Node* game);
		int getVoxel(Vector3 position);
		std::shared_ptr<GraphNode> getNode(Vector3 position);
		std::shared_ptr<Chunk> getChunk(int x, int z);
		std::shared_ptr<Chunk> getChunk(int i);
		void setChunk(int x, int z, std::shared_ptr<Chunk> chunk);
		void setChunk(int i, std::shared_ptr<Chunk> chunk);
		void fill(EcoGame* lib, int sectionSize);
		void build(std::shared_ptr<ChunkBuilder> builder, Node* game);
		Vector2 getOffset();
		void buildAreasByType(VoxelAssetType type);
		void buildArea(Area area, VoxelAssetType type);
		vector<Area> findAreasOfSize(int size, int* mask, int* surfaceY, int* sub);
	};
}

#endif