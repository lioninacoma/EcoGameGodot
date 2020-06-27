#ifndef CHUNKBUILDER_H
#define CHUNKBUILDER_H

#include <Spatial.hpp>
#include <String.hpp>
#include <Array.hpp>
#include <MeshInstance.hpp>
#include <ArrayMesh.hpp>

#include <deque>
#include <unordered_set>
#include <vector>
#include <iostream>

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include "constants.h"
#include "chunk.h"
#include "meshbuilder.h"
#include "objectpool.h"
#include "fn.h"

using namespace std;
namespace bpt = boost::posix_time;

#define CHUNKBUILDER_VERTEX_SIZE 3
#define CHUNKBUILDER_FACE_SIZE 4
#define CHUNKBUILDER_MAX_VERTICES 16000
#define CHUNKBUILDER_MAX_FACES CHUNKBUILDER_MAX_VERTICES / 3

namespace godot {

	class VoxelWorld;

	class ChunkBuilder {

	private:
		void processQueue();
		void queueChunk(std::shared_ptr<Chunk> chunk);
		void buildChunk(std::shared_ptr<Chunk> chunk);
		void buildMesh(std::shared_ptr<Chunk> chunk, std::shared_ptr<OctreeNode> seam);
		void addWaiting(size_t notifying, std::shared_ptr<Chunk> chunk);
		void queueWaiting(size_t notifying);

		std::shared_ptr<VoxelWorld> world;
		std::unique_ptr<MeshBuilder> meshBuilder;
		std::unique_ptr<boost::thread> queueThread;
		deque<std::shared_ptr<Chunk>> buildQueue;
		unordered_set<size_t> inque;
		unordered_map<size_t, unordered_set<std::shared_ptr<Chunk>>*> waitingQueue;
		unordered_map<size_t, int> edgeCounts;
		unordered_map<size_t, int> nodeCounts;

		boost::mutex BUILD_QUEUE_MUTEX;
		boost::mutex BUILD_QUEUE_WAIT;
		boost::mutex BUILD_MESH_MUTEX;
		boost::condition_variable BUILD_QUEUE_CV;
		std::atomic<bool> threadStarted = false;
		int amountFacesTotal = 0;
	public:
		ChunkBuilder(std::shared_ptr<VoxelWorld> world);
		~ChunkBuilder() {
			queueThread->interrupt();
			queueThread->join();
		};
		void build(std::shared_ptr<Chunk> chunk);
	};

}

#endif