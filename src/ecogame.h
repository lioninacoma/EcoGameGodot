#ifndef ECOGAME_H
#define ECOGAME_H

#include <Godot.hpp>
#include <SceneTree.hpp>
#include <ViewPort.hpp>
#include <Node.hpp>
#include <String.hpp>
#include <Array.hpp>
#include <Vector3.hpp>

#include <iostream>
#include <iomanip>
#include <set> 
#include <iterator> 

#include "constants.h"
#include "chunkbuilder.h"

#include "voxelassetmanager.h"

using namespace std;

#define INT_POOL_BUFFER_SIZE CHUNK_SIZE_X * CHUNK_SIZE_Z * 128

namespace godot {
	class EcoGame : public Node {
		GODOT_CLASS(EcoGame, Node)

	private:
		ChunkBuilder chunkBuilder;

		class Area {
		private:
			Vector2 start;
			Vector2 end;
			Vector2 offset;
			float y;
		public:
			Area() : Area(Vector2(0, 0), Vector2(0, 0), 0) {};
			Area(Vector2 start, Vector2 end, float y) {
				Area::start = start;
				Area::end = end;
				Area::y = y;
			}

			void setOffset(Vector2 offset) {
				Area::offset = offset;
			}

			Vector2 getOffset() {
				return offset;
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

			float getWidth() {
				return (Area::end.x - Area::start.x);
			}

			float getHeight() {
				return (Area::end.y - Area::start.y);
			}
		};

		static ObjectPool<int, INT_POOL_BUFFER_SIZE, POOL_SIZE>& getIntBufferPool() {
			static ObjectPool<int, INT_POOL_BUFFER_SIZE, POOL_SIZE> pool;
			return pool;
		};

		vector<EcoGame::Area> findAreasOfSize(int size, int* mask, int* surfaceY, const int W, const int H);
		void buildArea(EcoGame::Area area, Chunk** chunks, VoxelAssetType type, int xS, int zS, const int C_W, const int W, const int CHUNKS_LEN);
		void buildAreasJob(Vector2 center, float radius, Node* game);
		void buildAreasByType(Vector2 center, float radius, VoxelAssetType type, vector<int>* indices, Node* game);
	public:
		static void _register_methods();

		EcoGame();
		~EcoGame();

		void _init();

		void buildChunk(Variant vChunk);
		Chunk* getChunk(int x, int z);

		void buildAreas(Vector2 center, float radius);
	};

}

#endif
