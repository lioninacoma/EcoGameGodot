#ifndef CHUNKBUILDER_H
#define CHUNKBUILDER_H

#include <Spatial.hpp>
#include <String.hpp>
#include <Array.hpp>
#include <MeshInstance.hpp>
#include <ArrayMesh.hpp>

#include <deque>
#include <vector>
#include <iostream>
#include <shared_mutex>
#include <mutex>
#include <condition_variable>

#include <boost/date_time/posix_time/posix_time_types.hpp>

#include "constants.h"
#include "chunk.h"
#include "meshbuilder.h"
#include "objectpool.h"

using namespace std;
namespace bpt = boost::posix_time;

namespace godot {

	class ChunkBuilder {

	private:
		class Worker {
		private:
			static ObjectPool<float, MAX_VERTICES_SIZE, TYPES * MAX_CHUNKS_BUILT_ASYNCH>& getVerticesPool() {
				static ObjectPool<float, MAX_VERTICES_SIZE, TYPES * MAX_CHUNKS_BUILT_ASYNCH> pool;
				return pool;
			};
			MeshBuilder meshBuilder;
		public:
			void run(std::shared_ptr<Chunk> chunk, Node* game, ChunkBuilder* builder);
		};
		
		deque<std::shared_ptr<Chunk>> buildQueue;
		unordered_set<size_t> inque;
		void processQueue(Node* game);
		void queueChunk(std::shared_ptr<Chunk> chunk);

		std::shared_timed_mutex BUILD_QUEUE_MUTEX;
		std::mutex BUILD_QUEUE_WAIT;
		std::condition_variable BUILD_QUEUE_WAIT_CV;
		std::atomic<bool> threadStarted = false;
	public:
		ChunkBuilder();
		~ChunkBuilder() {};
		void build(std::shared_ptr<Chunk> chunk, Node* game);
	};

}

#endif