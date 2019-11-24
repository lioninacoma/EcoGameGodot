#ifndef WORLDBUILDER_H
#define WORLDBUILDER_H

#include <Godot.hpp>
#include <Node.hpp>
#include <String.hpp>
#include <Array.hpp>

#include <iostream>

#include "constants.h"
#include "chunkbuilder.h"
#include "scenehandle.h"
#include "objectpool.h"

using namespace std;

namespace godot {

	class WorldBuilder : public Reference {
		GODOT_CLASS(WorldBuilder, Reference)

	private:
		static ObjectPool<int, CHUNK_SIZE_X * CHUNK_SIZE_Z * 16, POOL_SIZE>& getIntBufferPool() {
			static ObjectPool<int, CHUNK_SIZE_X * CHUNK_SIZE_Z * 16, POOL_SIZE> pool;
			return pool;
		};

		PoolVector2Array findNextSquare(int* mask, const int W, const int H);
		void markSquare(PoolVector2Array rect, int* mask, const int W);
	public:
		static void _register_methods();

		WorldBuilder();
		~WorldBuilder();

		void _init();

		Chunk* getChunk(int x, int z);
		Array getSquares(Vector2 center, float radius, float minSideLength);
		
	};
}

#endif