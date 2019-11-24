#ifndef ECOGAME_H
#define ECOGAME_H

#include <Godot.hpp>
#include <SceneTree.hpp>
#include <ViewPort.hpp>
#include <Node.hpp>
#include <String.hpp>
#include <Array.hpp>

#include <iostream>
#include <iomanip>

#include "constants.h"
#include "chunkbuilder.h"

using namespace std;

namespace godot {

	class EcoGame : public Node {
		GODOT_CLASS(EcoGame, Node)

	private:
		static ObjectPool<int, CHUNK_SIZE_X * CHUNK_SIZE_Z * 64, POOL_SIZE>& getIntBufferPool() {
			static ObjectPool<int, CHUNK_SIZE_X * CHUNK_SIZE_Z * 64, POOL_SIZE> pool;
			return pool;
		};

		PoolVector2Array findNextSquare(int* mask, const int W, const int H);
		void markSquare(PoolVector2Array rect, int* mask, const int W);

		ChunkBuilder chunkBuilder;
	public:
		static void _register_methods();

		EcoGame();
		~EcoGame();

		void _init();
		void ready();

		void buildChunk(Variant vChunk);
		Chunk* getChunk(int x, int z);
		Array getSquares(Vector2 center, float radius, float minSideLength);
	};

}

#endif
