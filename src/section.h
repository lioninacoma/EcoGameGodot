#ifndef SECTION_H
#define SECTION_H

#include <Godot.hpp>
#include <Reference.hpp>
#include <Vector3.hpp>
#include <Texture.hpp>
#include <OpenSimplexNoise.hpp>

#include <iostream>
#include <string>

#include "voxelassetmanager.h"
#include "constants.h"
#include "chunkbuilder.h"
#include "fn.h"
#include "chunk.h"

using namespace std;

namespace godot {

	class Section : public Reference {
		GODOT_CLASS(Section, Reference)

	private:
		Chunk** chunks;
		Vector2 offset;
		int sectionSize;
		int sectionChunksLen;
		OpenSimplexNoise* noise;
	public:
		static ObjectPool<int, INT_POOL_BUFFER_SIZE, 4>& getIntBufferPool() {
			static ObjectPool<int, INT_POOL_BUFFER_SIZE, 4> pool;
			return pool;
		};

		static void _register_methods() {
			register_method("getOffset", &Section::getOffset);
		}

		Section() : Section(Vector2()) {};
		Section(Vector2 offset) : Section(offset, SECTION_SIZE) {};
		Section(Vector2 offset, int sectionSize) {
			Section::offset = offset;
			Section::sectionSize = sectionSize;
			Section::sectionChunksLen = sectionSize * sectionSize;
			chunks = new Chunk * [sectionChunksLen * sizeof(*chunks)];

			memset(chunks, 0, sectionChunksLen * sizeof(*chunks));

			Section::noise = OpenSimplexNoise::_new();
			Section::noise->set_seed(NOISE_SEED);
			Section::noise->set_octaves(3);
			Section::noise->set_period(60.0);
			Section::noise->set_persistence(0.5);
		};

		~Section() {
			delete[] chunks;
		}

		// our initializer called by Godot
		void _init() {

		}

		float getVoxelAssetChance(int x, int y, float scale) {
			return noise->get_noise_2d(
				(x + offset.x * CHUNK_SIZE_X) * scale,
				(y + offset.y * CHUNK_SIZE_Z) * scale) / 2.0 + 0.5;
		}

		void addVoxelAsset(Vector3 startV, VoxelAssetType type, ChunkBuilder* builder, Node* game) {
			VoxelAsset* voxelAsset = VoxelAssetManager::get()->getVoxelAsset(type);
			int i, areaSize = max(voxelAsset->getWidth(), voxelAsset->getHeight());
			Chunk* chunk;
			Vector2 start = Vector2(startV.x, startV.z);
			Vector2 end = start + Vector2(areaSize, areaSize);
			Area area = Area(start, end, startV.y);

			buildArea(area, type);

			for (i = 0; i < sectionChunksLen; i++) {
				chunk = chunks[i];
				if (!chunk) continue;
				builder->build(chunk, game);
			}
		}

		bool voxelAssetFits(Vector3 start, VoxelAssetType type) {
			VoxelAsset* voxelAsset = VoxelAssetManager::get()->getVoxelAsset(type);
			int maxDeltaY = voxelAsset->getMaxDeltaY();
			int areaSize = max(voxelAsset->getWidth(), voxelAsset->getHeight());
			int x, y, z, cx, cz, ci, dy;
			Chunk* chunk;
			Vector2 chunkOffset, end = Vector2(start.x, start.z) + Vector2(areaSize, areaSize);
			y = (int)start.y;
			
			for (z = start.z; z < end.y; z++) {
				for (x = start.x; x < end.x; x++) {
					chunkOffset.x = x;
					chunkOffset.y = z;
					chunkOffset = fn::toChunkCoords(chunkOffset);
					cx = (int)(chunkOffset.x - offset.x);
					cz = (int)(chunkOffset.y - offset.y);
					ci = fn::fi2(cx, cz, sectionSize);

					if (ci < 0 || ci >= sectionChunksLen) continue;

					chunk = chunks[ci];
					
					if (!chunk) continue;

					if (abs(chunk->getCurrentSurfaceY(
						x % CHUNK_SIZE_X,
						z % CHUNK_SIZE_Z) - y) > maxDeltaY) {
						return false;
					}
				}
			}

			return true;
		}

		void fill(Section** sections, int sectionSize, int sectionsSize) {
			int xS, xE, zS, zE, x, z, sx, sy, cx, cz, ci;
			Section* section;
			Chunk* chunk;
			Vector3 tl, br, chunkCoords;

			tl = Vector3(offset.x, 0, offset.y);
			br = tl + Vector3(Section::sectionSize, 0, Section::sectionSize);

			xS = int(tl.x);
			xS = max(0, xS);

			xE = int(br.x);
			xE = min(WORLD_SIZE - 1, xE);

			zS = int(tl.z);
			zS = max(0, zS);

			zE = int(br.z);
			zE = min(WORLD_SIZE - 1, zE);

			for (z = (int)(zS / sectionSize); z <= (int)(zE / sectionSize); z++) {
				for (x = (int)(xS / sectionSize); x <= (int)(xE / sectionSize); x++) {
					section = sections[fn::fi2(x, z, sectionsSize)];
					if (!section) continue;

					for (sy = 0; sy < sectionSize; sy++) {
						for (sx = 0; sx < sectionSize; sx++) {
							chunk = section->chunks[fn::fi2(sx, sy, sectionSize)];
							if (!chunk) continue;
							
							chunkCoords = chunk->getOffset();
							chunkCoords = fn::toChunkCoords(chunkCoords);

							if (chunkCoords.x >= xS && chunkCoords.z >= zS && chunkCoords.x < xE && chunkCoords.z < zE) {
								cx = (int)(chunkCoords.x - offset.x);
								cz = (int)(chunkCoords.z - offset.y);
								ci = fn::fi2(cx, cz, Section::sectionSize);
								chunks[ci] = chunk;
							}
						}
					}
				}
			}
		}

		void build(ChunkBuilder* builder, Node* game) {
			int x, y, i;
			Chunk* chunk;

			for (y = 0; y < sectionSize; y++) {
				for (x = 0; x < sectionSize; x++) {
					chunk = Chunk::_new();
					chunk->setOffset(Vector3((x + offset.x) * CHUNK_SIZE_X, 0, (y + offset.y) * CHUNK_SIZE_Z));
					chunks[fn::fi2(x, y, sectionSize)] = chunk;
					chunk->buildVolume();
				}
			}

			buildAreasByType(VoxelAssetType::PINE_TREE);
			buildAreasByType(VoxelAssetType::HOUSE_6X6);
			buildAreasByType(VoxelAssetType::HOUSE_4X4);
			
			
			for (i = 0; i < sectionChunksLen; i++) {
				chunk = chunks[i];
				if (!chunk) continue;
				builder->build(chunk, game);
			}
		}

		Vector2 getOffset() {
			return Section::offset;
		}

		Chunk* getChunk(int x, int y) {
			if (x < 0 || x >= sectionSize) return NULL;
			if (y < 0 || y >= sectionSize) return NULL;
			return chunks[fn::fi2(x, y, sectionSize)];
		}

		Chunk* getChunk(int i) {
			if (i < 0 || i >= sectionChunksLen) return NULL;
			return chunks[i];
		}

		void buildAreasByType(VoxelAssetType type) {
			VoxelAsset* voxelAsset = VoxelAssetManager::get()->getVoxelAsset(type);
			int maxDeltaY = voxelAsset->getMaxDeltaY();
			int areaSize = max(voxelAsset->getWidth(), voxelAsset->getHeight());
			float assetNoiseChance = voxelAsset->getNoiseChance();
			float assetNoiseOffset = voxelAsset->getNoiseOffset();
			float assetNoiseScale = voxelAsset->getNoiseScale();
			int currentY, currentType, currentIndex, deltaY, lastY = -1, ci, i, j, it, vx, vz;
			Vector2 areasOffset, start;
			Vector3 chunkOffset;
			Chunk* chunk;
			vector<Area> areas;
			vector<int> yValues;

			areasOffset.x = offset.x * CHUNK_SIZE_X; // Voxel
			areasOffset.y = offset.y * CHUNK_SIZE_Z; // Voxel

			int** buffers = getIntBufferPool().borrow(4);

			for (int i = 0; i < 4; i++)
				memset(buffers[i], 0, INT_POOL_BUFFER_SIZE * sizeof(*buffers[i]));

			int* surfaceY = buffers[0];
			int* mask = buffers[1];
			int* types = buffers[2];
			int* sub = buffers[3];

			const int SECTION_SIZE_CHUNKS = sectionSize * CHUNK_SIZE_X;

			for (it = 0; it < sectionChunksLen; it++) {
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

				areas = findAreasOfSize(areaSize, mask, surfaceY, sub);

				for (Area area : areas) {
					start = area.getStart();
					area.setOffset(areasOffset);
					if (getVoxelAssetChance(start.x + assetNoiseOffset, start.y + assetNoiseOffset, assetNoiseScale) > assetNoiseChance) continue;
					buildArea(area, type);
					//Godot::print(String("area start: {0}, end: {1}").format(Array::make(area.getStart(), area.getEnd())));
				}
			}

			getIntBufferPool().ret(buffers, 4);
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
				ci = fn::fi2(cx, cz, sectionSize);

				if (ci < 0 || ci >= sectionChunksLen) continue;

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

		Area* findNextArea(int* mask, int* surfaceY) {
			const int SECTION_SIZE_CHUNKS = sectionSize * CHUNK_SIZE_X;
			int i, j, x0, x1, y0, y1;
			int max_of_s, max_i, max_j;

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
					}
					else {
						sub[fn::fi2(i, j, SECTION_SIZE_CHUNKS)] = 0;
					}
				}
			}

			max_of_s = sub[fn::fi2(0, 0)];
			max_i = 0;
			max_j = 0;

			for (i = 0; i < SECTION_SIZE_CHUNKS; i++) {
				for (j = 0; j < SECTION_SIZE_CHUNKS; j++) {
					if (max_of_s < sub[fn::fi2(i, j)]) {
						max_of_s = sub[fn::fi2(i, j)];
						max_i = i;
						max_j = j;
					}
				}
			}

			x0 = (max_i - max_of_s) + 1;
			x1 = max_i + 1;
			y0 = (max_j - max_of_s) + 1;
			y1 = max_j + 1;
			
			getIntBufferPool().ret(sub);

			return new Area(Vector2(x0, y0), Vector2(x1, y1), surfaceY[fn::fi2(x0, y0)]);
		}

		vector<Area> findAreasOfSize(int size, int* mask, int* surfaceY, int* sub) {
			int i, j, x, y, x0, x1, y0, y1, areaY, currentSize, meanY = 0, count = 0;
			vector<Area> areas;

			const int SECTION_SIZE_CHUNKS = sectionSize * CHUNK_SIZE_X;

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
							x0 = (i - currentSize) + 1;
							y0 = (j - currentSize) + 1;
							x1 = i + 1;
							y1 = j + 1;

							meanY = 0;
							count = 0;

							for (y = y0; y < y1; y++) {
								for (x = x0; x < x1; x++) {
									sub[fn::fi2(x, y, SECTION_SIZE_CHUNKS)] = 0;
									meanY += surfaceY[fn::fi2(x, y, SECTION_SIZE_CHUNKS)];
									count++;
								}
							}

							areaY = (int)((float)meanY / (float)count);
							areas.push_back(Area(Vector2(x0, y0), Vector2(x1, y1), areaY));
						}
					}
					else {
						sub[fn::fi2(i, j, SECTION_SIZE_CHUNKS)] = 0;
					}
				}
			}

			return areas;
		}
	};
}

#endif