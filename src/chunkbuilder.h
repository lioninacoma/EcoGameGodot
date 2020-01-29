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

#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_types.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include "constants.h"
#include "chunk.h"
#include "meshbuilder.h"
#include "objectpool.h"
#include "threadpool.h"

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
			void run(boost::shared_ptr<Chunk> chunk, Node* game);
		};
	public:
		ChunkBuilder() {};
		~ChunkBuilder() {};
		void build(boost::shared_ptr<Chunk> chunk, Node* game);
	};

}

#endif