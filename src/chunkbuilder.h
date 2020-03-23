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

#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include "constants.h"
#include "chunk.h"
#include "meshbuilder.h"
#include "objectpool.h"

using namespace std;
namespace bpt = boost::posix_time;

#define CHUNKBUILDER_VERTEX_SIZE 3
#define CHUNKBUILDER_FACE_SIZE 4
#define CHUNKBUILDER_MAX_VERTICES (BUFFER_SIZE * 6 * 4) / 2
#define CHUNKBUILDER_MAX_FACES CHUNKBUILDER_MAX_VERTICES / 3

namespace godot {

	class ChunkBuilder {

	private:
		std::shared_ptr<boost::thread> queueThread;
		deque<std::shared_ptr<Chunk>> buildQueue;
		unordered_set<size_t> inque;
		void processQueue();
		void queueChunk(std::shared_ptr<Chunk> chunk);
		void buildChunk(std::shared_ptr<Chunk> chunk);

		MeshBuilder meshBuilder;
		boost::mutex BUILD_QUEUE_MUTEX;
		boost::mutex BUILD_QUEUE_WAIT;
		boost::mutex BUILD_MESH_MUTEX;
		boost::condition_variable BUILD_QUEUE_CV;
		std::atomic<bool> threadStarted = false;
	public:
		ChunkBuilder();
		~ChunkBuilder() {
			queueThread->interrupt();
			queueThread->join();
		};
		void build(std::shared_ptr<Chunk> chunk);
	};

}

#endif