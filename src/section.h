#ifndef SECTION_H
#define SECTION_H

#include <Godot.hpp>
#include <Reference.hpp>
#include <Vector3.hpp>

#include "voxelassetmanager.h"
#include "constants.h"
#include "chunkbuilder.h"
#include "fn.h"
#include "chunk.h"

namespace godot {

	class Section : public Reference {
		GODOT_CLASS(Section, Reference)

	private:
		Chunk** chunks;
		Vector2 offset;
	public:
		static ObjectPool<int, INT_POOL_BUFFER_SIZE, POOL_SIZE * 4>& getIntBufferPool() {
			static ObjectPool<int, INT_POOL_BUFFER_SIZE, POOL_SIZE * 4> pool;
			return pool;
		};

		static void _register_methods() {
			register_method("getOffset", &Section::getOffset);
		}

		Section() : Section(Vector2()) {};
		Section(Vector2 offset) {
			Section::offset = offset;
			chunks = new Chunk * [SECTION_CHUNKS_LEN * sizeof(*chunks)];

			memset(chunks, 0, SECTION_CHUNKS_LEN * sizeof(*chunks));
		}

		~Section() {
			delete[] chunks;
		}

		// our initializer called by Godot
		void _init() {

		}

		void build(ChunkBuilder* builder, Node* game) {
			int x, y, i;
			Chunk* chunk;

			for (y = 0; y < SECTION_SIZE; y++) {
				for (x = 0; x < SECTION_SIZE; x++) {
					chunk = Chunk::_new();
					chunk->setOffset(Vector3((x + offset.x) * CHUNK_SIZE_X, 0, (y + offset.y) * CHUNK_SIZE_Z));
					chunks[fn::fi2(x, y, SECTION_SIZE)] = chunk;
					chunk->buildVolume();
				}
			}

			buildAreasByType(VoxelAssetType::HOUSE_6X6);
			buildAreasByType(VoxelAssetType::HOUSE_4X4);
			buildAreasByType(VoxelAssetType::PINE_TREE);

			for (i = 0; i < SECTION_CHUNKS_LEN; i++) {
				chunk = chunks[i];
				if (!chunk) continue;
				builder->build(chunk, game);
			}
		}

		Vector2 getOffset() {
			return Section::offset;
		}

		Chunk* getChunk(int x, int y) {
			if (x < 0 || x >= SECTION_SIZE) return NULL;
			if (y < 0 || y >= SECTION_SIZE) return NULL;
			return chunks[fn::fi2(x, y, SECTION_SIZE)];
		}

		Chunk* getChunk(int i) {
			if (i < 0 || i >= SECTION_CHUNKS_LEN) return NULL;
			return chunks[i];
		}

		void buildAreasByType(VoxelAssetType type) {
			VoxelAsset* voxelAsset = VoxelAssetManager::get()->getVoxelAsset(type);
			int maxDeltaY = voxelAsset->getMaxDeltaY();
			int areaSize = max(voxelAsset->getWidth(), voxelAsset->getHeight());
			int currentY, currentType, currentIndex, deltaY, lastY = -1, ci, i, j, it, vx, vz;
			Vector2 areasOffset;
			Vector3 chunkOffset;
			Chunk* chunk;
			vector<Area> areas;
			vector<int> yValues;

			areasOffset.x = offset.x * CHUNK_SIZE_X; // Voxel
			areasOffset.y = offset.y * CHUNK_SIZE_Z; // Voxel

			//Godot::print(String("areasOffset: {0}").format(Array::make(areasOffset)));
			//return;

			int* surfaceY = getIntBufferPool().borrow();
			memset(surfaceY, 0, INT_POOL_BUFFER_SIZE * sizeof(*surfaceY));

			int* mask = getIntBufferPool().borrow();
			memset(mask, 0, INT_POOL_BUFFER_SIZE * sizeof(*mask));

			int* types = getIntBufferPool().borrow();
			memset(types, 0, INT_POOL_BUFFER_SIZE * sizeof(*types));

			//Godot::print(String("chunks, surfaceY and mask initialized"));

			const int SECTION_SIZE_CHUNKS = SECTION_SIZE * CHUNK_SIZE_X;

			for (it = 0; it < SECTION_CHUNKS_LEN; it++) {
				chunk = chunks[it];

				if (!chunk) continue;

				chunkOffset = chunk->getOffset();				// Voxel
				chunkOffset = fn::toChunkCoords(chunkOffset);	// Chunks

				//Godot::print(String("chunkOffset: {0}").format(Array::make(chunkOffset)));

				for (j = 0; j < CHUNK_SIZE_Z; j++) {
					for (i = 0; i < CHUNK_SIZE_X; i++) {
						vx = (int)((chunkOffset.x - offset.x) * CHUNK_SIZE_X + i);
						vz = (int)((chunkOffset.z - offset.y) * CHUNK_SIZE_Z + j);

						//Godot::print(String("vPos: {0}").format(Array::make(vPos)));

						currentY = chunk->getCurrentSurfaceY(i, j);
						currentType = chunk->getVoxel(i, currentY, j);
						currentIndex = fn::fi2(vx, vz, SECTION_SIZE_CHUNKS);
						yValues.push_back(currentY);
						surfaceY[currentIndex] = currentY;
						types[currentIndex] = currentType;
					}
				}
			}

			std::sort(yValues.begin(), yValues.end(), std::greater<int>());
			auto last = std::unique(yValues.begin(), yValues.end());
			yValues.erase(last, yValues.end());

			//Godot::print(String("surfaceY and uniques set"));

			for (int nextY : yValues) {

				if (lastY >= 0 && lastY - nextY <= maxDeltaY) continue;
				lastY = nextY;

				//Godot::print(String("nextY: {0}").format(Array::make(nextY)));

				for (i = 0; i < SECTION_SIZE_CHUNKS * SECTION_SIZE_CHUNKS; i++) {
					deltaY = nextY - surfaceY[i];
					currentType = types[i];
					mask[i] = (deltaY <= maxDeltaY && deltaY >= 0 && currentType != 6) ? 1 : 0;
				}

				areas = findAreasOfSize(areaSize, mask, surfaceY);

				for (Area area : areas) {
					area.setOffset(areasOffset);
					buildArea(area, type);
					//Godot::print(String("area start: {0}, end: {1}").format(Array::make(area.getStart(), area.getEnd())));
				}
			}

			getIntBufferPool().ret(surfaceY);
			getIntBufferPool().ret(mask);
			getIntBufferPool().ret(types);
		}

		void buildArea(Area area, VoxelAssetType type) {
			int voxelType, vx, vy, vz, ci, cx, cz, ay = area.getY() + 1;
			Chunk* chunk;
			Vector2 chunkOffset;
			Vector2 start = area.getStart() + area.getOffset();
			vector<Voxel>* voxels = VoxelAssetManager::get()->getVoxels(type);

			if (!voxels) return;

			Vector3 pos;
			Vector3 voxelOffset = Vector3(start.x, ay, start.y);

			for (vector<Voxel>::iterator it = voxels->begin(); it != voxels->end(); it++) {
				Voxel v = *it;
				pos = v.getPosition();
				pos += voxelOffset;
				voxelType = v.getType();
				chunkOffset.x = pos.x;
				chunkOffset.y = pos.z;
				chunkOffset = fn::toChunkCoords(chunkOffset);
				cx = (int)(chunkOffset.x - offset.x);
				cz = (int)(chunkOffset.y - offset.y);
				ci = fn::fi2(cx, cz, SECTION_SIZE);

				if (ci < 0 || ci >= SECTION_CHUNKS_LEN) continue;

				chunk = chunks[ci];

				if (!chunk) continue;

				vx = (int)pos.x;
				vy = (int)pos.y;
				vz = (int)pos.z;

				chunk->setVoxel(
					vx % CHUNK_SIZE_X,
					vy % CHUNK_SIZE_Y,
					vz % CHUNK_SIZE_Z, voxelType);

				chunk->markAssetsBuilt();
			}
		}

		vector<Area> findAreasOfSize(int size, int* mask, int* surfaceY) {
			int i, j, x, y, x1, x2, y1, y2, areaY, currentSize, meanY = 0, count = 0;
			vector<Area> areas;

			const int SECTION_SIZE_CHUNKS = SECTION_SIZE * CHUNK_SIZE_X;

			int* sub = getIntBufferPool().borrow();
			memset(sub, 0, INT_POOL_BUFFER_SIZE * sizeof(*sub));

			for (i = 0; i < SECTION_SIZE_CHUNKS; i++)
				sub[fn::fi2(i, 0, SECTION_SIZE_CHUNKS)] = mask[fn::fi2(i, 0, SECTION_SIZE_CHUNKS)];

			for (j = 0; j < SECTION_SIZE_CHUNKS; j++)
				sub[fn::fi2(0, j, SECTION_SIZE_CHUNKS)] = mask[fn::fi2(0, j, SECTION_SIZE_CHUNKS)];

			for (i = 1; i < SECTION_SIZE_CHUNKS; i++) {
				for (j = 1; j < SECTION_SIZE_CHUNKS; j++) {
					if (mask[fn::fi2(i, j, SECTION_SIZE_CHUNKS)] > 0) {
						sub[fn::fi2(i, j, SECTION_SIZE_CHUNKS)] = min(
							sub[fn::fi2(i, j - 1, SECTION_SIZE_CHUNKS)],
							min(sub[fn::fi2(i - 1, j, SECTION_SIZE_CHUNKS)],
								sub[fn::fi2(i - 1, j - 1, SECTION_SIZE_CHUNKS)])) + 1;

						currentSize = sub[fn::fi2(i, j, SECTION_SIZE_CHUNKS)];

						if (currentSize == size) {
							x1 = (i - currentSize) + 1;
							y1 = (j - currentSize) + 1;
							x2 = i + 1;
							y2 = j + 1;

							meanY = 0;
							count = 0;

							for (y = y1; y < y2; y++) {
								for (x = x1; x < x2; x++) {
									sub[fn::fi2(x, y, SECTION_SIZE_CHUNKS)] = 0;
									meanY += surfaceY[fn::fi2(x, y, SECTION_SIZE_CHUNKS)];
									count++;
								}
							}

							areaY = (int)((float)meanY / (float)count);
							areas.push_back(Area(Vector2(x1, y1), Vector2(x2, y2), areaY));
						}
					}
					else {
						sub[fn::fi2(i, j, SECTION_SIZE_CHUNKS)] = 0;
					}
				}
			}

			getIntBufferPool().ret(sub);
			return areas;
		}
	};
}

#endif