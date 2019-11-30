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
#include <set> 
#include <iterator> 

#include "constants.h"
#include "chunkbuilder.h"

using namespace std;

namespace godot {

	class EcoGame : public Node {
		GODOT_CLASS(EcoGame, Node)

	private:
		class Area {
		private:
			Vector2 start;
			Vector2 end;
			float y;
		public:
			Area() : Area(Vector2(0, 0), Vector2(0, 0), 0) {};
			Area(Vector2 start, Vector2 end, float y) {
				Area::start = start;
				Area::end = end;
				Area::y = y;
			}

			void addOffset(Vector2 offset) {
				start += offset;
				end += offset;
			}

			Vector2 getStart() {
				return Area::start;
			}

			Vector2 getEnd() {
				return Area::end;
			}

			float getY() {
				return Area::y;
			}
		};

		static ObjectPool<int, CHUNK_SIZE_X * CHUNK_SIZE_Z * 64, POOL_SIZE>& getIntBufferPool() {
			static ObjectPool<int, CHUNK_SIZE_X * CHUNK_SIZE_Z * 64, POOL_SIZE> pool;
			return pool;
		};

		//PoolVector2Array findNextSquare(int* mask, const int W, const int H);
		//void markSquare(PoolVector2Array rect, int* mask, const int W);

		EcoGame::Area findNextArea(int* mask, int y, const int W, const int H);
		void markArea(EcoGame::Area area, int* mask, int xS, int zS, const int W);
		void buildArea(EcoGame::Area area, vector<Chunk*> chunks, int xS, int zS, const int C_W);

		ChunkBuilder chunkBuilder;
		void buildAreasT(Vector2 center, float radius, float minSideLength, Node* game);
	public:
		static void _register_methods();

		EcoGame();
		~EcoGame();

		void _init();
		void ready();

		void buildChunk(Variant vChunk);
		Chunk* getChunk(int x, int z);

		void buildAreas(Vector2 center, float radius, float minSideLength);
		void buildArea();
		Array getSquares(Vector2 center, float radius, float minSideLength);
	};

}

#endif
