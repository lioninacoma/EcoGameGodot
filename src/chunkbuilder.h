#ifndef CHUNKBUILDER_H
#define CHUNKBUILDER_H

#include <Spatial.hpp>
#include <String.hpp>
#include <Array.hpp>
#include <MeshInstance.hpp>
#include <ArrayMesh.hpp>

#include <set>
#include <deque>
#include <vector>
#include <iostream>

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
			static ObjectPool<float, MAX_VERTICES_SIZE, 16>& getVerticesPool() {
				static ObjectPool<float, MAX_VERTICES_SIZE, 16> pool;
				return pool;
			};
			MeshBuilder meshBuilder;
		public:
			void run(Chunk* chunk, Node* game);
		};
	public:
		ChunkBuilder() {};
		~ChunkBuilder() {};
		void build(Chunk* chunk, Node* game);
	};

}

#endif