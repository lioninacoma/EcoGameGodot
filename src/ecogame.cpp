#include "ecogame.h"

using namespace godot;

void EcoGame::_register_methods() {
	register_method("buildChunk", &EcoGame::buildChunk);
	register_method("buildAreas", &EcoGame::buildAreas);
}

EcoGame::EcoGame() {
	
}

EcoGame::~EcoGame() {
	// add your cleanup here
}

void EcoGame::_init() {
	// initialize any variables here
}

void EcoGame::buildChunk(Variant vChunk) {
	Chunk* chunk = as<Chunk>(vChunk.operator Object * ());
	Node* game = get_tree()->get_root()->get_node("EcoGame");
	chunkBuilder.build(chunk, game);
}

Chunk* EcoGame::getChunk(int x, int z) {
	Chunk* chunk = NULL;
	Node* game = get_tree()->get_root()->get_node("EcoGame");
	Variant vChunk = game->call("get_chunk", x, z);

	if (vChunk) {
		chunk = as<Chunk>(vChunk.operator Object * ());
	}

	return chunk;
}

void EcoGame::buildAreas(Vector2 center, float radius) {
	Node* game = get_tree()->get_root()->get_node("EcoGame");
	ThreadPool::get()->submitTask(boost::bind(&EcoGame::buildAreasJob, this, center, radius, game));
}

void EcoGame::buildAreasJob(Vector2 center, float radius, Node* game) {
	auto indices = new std::vector<int>();

	buildAreasByType(center, radius, VoxelAssetType::HOUSE_6X6, indices, game);
	buildAreasByType(center, radius, VoxelAssetType::HOUSE_4X4, indices, game);
	buildAreasByType(center, radius, VoxelAssetType::PINE_TREE, indices, game);

	std::sort(indices->begin(), indices->end());
	auto last = std::unique(indices->begin(), indices->end());
	indices->erase(last, indices->end());

	Array indicesArray;
	for (std::vector<int>::iterator it = indices->begin(); it != indices->end(); it++) {
		indicesArray.push_back(*it);
	}

	Variant v = game->call_deferred("update_chunks", indicesArray, indicesArray.size());
}

void EcoGame::buildAreasByType(Vector2 center, float radius, VoxelAssetType type, vector<int>* indices, Node* game) {
	VoxelAsset* voxelAsset = VoxelAssetManager::get()->getVoxelAsset(type);
	int maxDeltaY = voxelAsset->getMaxDeltaY();
	int areaSize = voxelAsset->getWidth();
	int xS, xE, zS, zE, currentY, deltaY, lastY = -1, x, z, ci, i, j;
	Area area;
	Vector2 offset, vPos, centerNorm;
	Chunk* chunk;
	Chunk** chunks;
	vector<Area> areas;
	vector<int> yValues;
	
	Vector3 tl = Vector3(center.x - radius, 0, center.y - radius);
	Vector3 br = Vector3(center.x + radius, 0, center.y + radius);

	tl = fn::toChunkCoords(tl);
	br = fn::toChunkCoords(br);

	xS = (int)tl.x;
	xS = max(0, xS);

	xE = (int)br.x;
	xE = min(WORLD_SIZE - 1, xE);

	zS = (int)tl.z;
	zS = max(0, zS);

	zE = (int)br.z;
	zE = min(WORLD_SIZE - 1, zE);

	offset.x = xS * CHUNK_SIZE_X;
	offset.y = zS * CHUNK_SIZE_Z;
	centerNorm = center - offset;

	const int C_W = xE - xS + 1;
	const int C_H = zE - zS + 1;
	const int W = C_W * CHUNK_SIZE_X;
	const int H = C_H * CHUNK_SIZE_Z;
	const int CHUNKS_LEN = C_W * C_H;

	//Godot::print(String("start: {0}, end: {1}, C_W: {2}, C_H: {3}, W: {4}, H: {5}").format(Array::make(Vector2(xS, zS), Vector2(xE, zE), C_W, C_H, W, H)));
	
	chunks = new Chunk*[CHUNKS_LEN * sizeof(*chunks)];
	memset(chunks, 0, CHUNKS_LEN * sizeof(*chunks));

	int* surfaceY = getIntBufferPool().borrow();
	memset(surfaceY, 0, INT_POOL_BUFFER_SIZE * sizeof(*surfaceY));

	int* mask = getIntBufferPool().borrow();
	memset(mask, 0, INT_POOL_BUFFER_SIZE * sizeof(*mask));

	//Godot::print(String("chunks, surfaceY and mask initialized"));

	for (z = zS; z <= zE; z++) {
		for (x = xS; x <= xE; x++) {
			indices->push_back(fn::fi2(x, z, WORLD_SIZE));

			chunk = getChunk(x, z);
			ci = fn::fi2(x - xS, z - zS, C_W);

			if (!chunk) continue;
			if (ci < 0 || ci >= CHUNKS_LEN) continue;

			//Godot::print(String("x: {0}, z: {1}, x - xS: {2}, z - zS: {3}, index: {4}, size: {5}").format(Array::make(x, z, x - xS, z - zS, fn::fi2(x - xS, z - zS, C_W), C_W * C_H)));
			chunks[ci] = chunk;
			//Godot::print(String("chunk offset: {0} set").format(Array::make(chunk->getOffset())));

			for (j = 0; j < CHUNK_SIZE_Z; j++) {
				for (i = 0; i < CHUNK_SIZE_X; i++) {
					vPos.x = (x - xS) * CHUNK_SIZE_X + i;
					vPos.y = (z - zS) * CHUNK_SIZE_Z + j;
					if (vPos.distance_to(centerNorm) > radius) continue;
					currentY = chunk->getCurrentSurfaceY(i, j);
					yValues.push_back(currentY);
					surfaceY[fn::fi2((int)vPos.x, (int)vPos.y, W)] = currentY;
				}
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

		for (i = 0; i < W * H; i++) {
			deltaY = nextY - surfaceY[i];
			mask[i] = (deltaY <= maxDeltaY && deltaY >= 0) ? 1 : 0;
		}

		areas = findAreasOfSize(areaSize, mask, surfaceY, W, H);

		for (Area area : areas) {
			area.setOffset(offset);
			buildArea(area, chunks, type, xS, zS, C_W, W, CHUNKS_LEN);
		}
	}

	delete[] chunks;
	getIntBufferPool().ret(surfaceY);
	getIntBufferPool().ret(mask);
}

void EcoGame::buildArea(EcoGame::Area area, Chunk** chunks, VoxelAssetType type, int xS, int zS, const int C_W, const int W, const int CHUNKS_LEN) {
	int i, j, voxelType, vx, vy, vz, ci, cx, cz, ay = area.getY() + 1;
	Chunk* chunk;
	Vector2 chunkPos;
	Vector2 start = area.getStart() + area.getOffset();
	vector<Voxel>* voxels = VoxelAssetManager::get()->getVoxels(type);

	if (!voxels) return;

	Vector3 pos;
	Vector3 offset = Vector3(start.x, ay, start.y);

	for (vector<Voxel>::iterator it = voxels->begin(); it != voxels->end(); it++) {
		Voxel v = *it;
		pos = v.getPosition();
		pos += offset;
		voxelType = v.getType();
		chunkPos.x = pos.x;
		chunkPos.y = pos.z;
		chunkPos = fn::toChunkCoords(chunkPos);
		cx = (int)chunkPos.x;
		cz = (int)chunkPos.y;
		ci = fn::fi2(cx - xS, cz - zS, C_W);
		
		if (ci < 0 || ci >= CHUNKS_LEN) continue;

		chunk = chunks[ci];
		
		if (!chunk) continue;

		vx = (int)pos.x;
		vy = (int)pos.y;
		vz = (int)pos.z;

		chunk->setVoxel(
			vx % CHUNK_SIZE_X,
			vy % CHUNK_SIZE_Y,
			vz % CHUNK_SIZE_Z, voxelType);
	}
}

vector<EcoGame::Area> EcoGame::findAreasOfSize(int size, int* mask, int* surfaceY, const int W, const int H) {
	int i, j, x, y, x1, x2, y1, y2, areaY, currentSize, meanY = 0, count = 0;
	vector<EcoGame::Area> areas;

	int* sub = getIntBufferPool().borrow();
	memset(sub, 0, INT_POOL_BUFFER_SIZE * sizeof(*sub));

	for (i = 0; i < W; i++)
		sub[fn::fi2(i, 0, W)] = mask[fn::fi2(i, 0, W)];

	for (j = 0; j < H; j++)
		sub[fn::fi2(0, j, W)] = mask[fn::fi2(0, j, W)];

	for (i = 1; i < W; i++) {
		for (j = 1; j < H; j++) {
			if (mask[fn::fi2(i, j, W)] > 0) {
				sub[fn::fi2(i, j, W)] = min(
					sub[fn::fi2(i, j - 1, W)],
					min(sub[fn::fi2(i - 1, j, W)],
						sub[fn::fi2(i - 1, j - 1, W)])) + 1;
				
				currentSize = sub[fn::fi2(i, j, W)];

				if (currentSize == size) {
					x1 = (i - currentSize) + 1;
					y1 = (j - currentSize) + 1;
					x2 = i + 1;
					y2 = j + 1;

					meanY = 0;
					count = 0;

					for (y = y1; y < y2; y++) {
						for (x = x1; x < x2; x++) {
							sub[fn::fi2(x, y, W)] = 0;
							meanY += surfaceY[fn::fi2(x, y, W)];
							count++;
						}
					}

					areaY = (int)((float)meanY / (float)count);
					areas.push_back(EcoGame::Area(Vector2(x1, y1), Vector2(x2, y2), areaY));
				}
			}
			else {
				sub[fn::fi2(i, j, W)] = 0;
			}
		}
	}

	getIntBufferPool().ret(sub);

	return areas;
}
