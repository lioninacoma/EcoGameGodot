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

void EcoGame::buildAreas(Vector2 center, float radius, float minSideLength) {
	if (minSideLength < 1) return;

	Node* game = get_tree()->get_root()->get_node("EcoGame");
	ThreadPool::get()->submitTask(boost::bind(&EcoGame::buildAreasT, this, center, radius, minSideLength, 1, game));
}

void EcoGame::buildAreasT(Vector2 center, float radius, float minSideLength, int maxDeltaY, Node* game) {
	int xS, xE, zS, zE, currentY, deltaY, lastY, x, z, i, j, totalSquareArea;
	float nextArea;
	Area area;
	Vector2 start, end, offset;
	Array indices;
	Chunk* chunk;
	vector<Chunk*> chunks;
	vector<Area> areas;
	std::set<int, greater<int>> uniques;

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

	const float MIN_SIZE = minSideLength * minSideLength;
	const int C_W = xE - xS + 1;
	const int C_H = zE - zS + 1;
	const int W = C_W * CHUNK_SIZE_X;
	const int H = C_H * CHUNK_SIZE_Z;

	//Godot::print(String("start: {0}, end: {1}, C_W: {2}, C_H: {3}, W: {4}, H: {5}").format(Array::make(Vector2(xS, zS), Vector2(xE, zE), C_W, C_H, W, H)));

	chunks.resize(C_W * C_H);
	int* surfaceY = getIntBufferPool().borrow();
	int* mask = getIntBufferPool().borrow();

	//Godot::print(String("chunks, surfaceY and mask initialized"));

	for (z = zS; z <= zE; z++) {
		for (x = xS; x <= xE; x++) {
			indices.push_back(fn::fi2(x, z, WORLD_SIZE));
			chunk = getChunk(x, z);

			if (!chunk) continue;

			//Godot::print(String("x: {0}, z: {1}, x - xS: {2}, z - zS: {3}, index: {4}, size: {5}").format(Array::make(x, z, x - xS, z - zS, fn::fi2(x - xS, z - zS, C_W), C_W * C_H)));
			chunks[fn::fi2(x - xS, z - zS, C_W)] = chunk;
			//Godot::print(String("chunk offset: {0} set").format(Array::make(chunk->getOffset())));

			for (j = 0; j < CHUNK_SIZE_Z; j++) {
				for (i = 0; i < CHUNK_SIZE_X; i++) {
					currentY = chunk->getCurrentSurfaceY(i, j);
					uniques.insert(currentY);
					surfaceY[fn::fi2((x - xS) * CHUNK_SIZE_X + i, (z - zS) * CHUNK_SIZE_Z + j, W)] = currentY;
				}
			}
		}
	}

	lastY = -1;

	//Godot::print(String("surfaceY and uniques set"));

	for (int nextY : uniques) {
		if (lastY >= 0 && lastY - nextY <= maxDeltaY) continue;
		lastY = nextY;

		//Godot::print(String("nextY: {0}").format(Array::make(nextY)));

		for (i = 0; i < W * H; i++) {
			deltaY = nextY - surfaceY[i];
			mask[i] = (deltaY <= maxDeltaY && deltaY >= 0) ? 1 : 0;
		}

		areas = findAreasOfSize(6, mask, surfaceY, W, H);

		for (Area area : areas) {
			start = area.getStart();
			end = area.getEnd();
			nextArea = area.getWidth() * area.getHeight();
			offset.x = xS * CHUNK_SIZE_X;
			offset.y = zS * CHUNK_SIZE_Z;
			area.setOffset(offset);

			buildArea(area, chunks, xS, zS, C_W, W);
		}
	}

	Variant v = game->call_deferred("update_chunks", indices, indices.size());

	getIntBufferPool().ret(surfaceY);
	getIntBufferPool().ret(mask);
}

void EcoGame::buildArea(EcoGame::Area area, vector<Chunk*> chunks, int xS, int zS, const int C_W, const int W) {
	int i, j, type, x, z, vx, vy, vz, y = area.getY() + 1, sx, sy, ex, ey;
	Chunk* chunk;
	Vector2 chunkPos;
	Vector2 start = area.getStart() + area.getOffset();
	vector<Voxel> voxels;

	//Godot::print(String("width: {0}").format(Array::make(area.getWidth())));
	
	if (area.getWidth() >= 6) {
		VoxelAsset_House6x6 house6x6;
		voxels = house6x6.getVoxels();
	}
	else if (area.getWidth() >= 4) {
		VoxelAsset_House4x4 house4x4;
		voxels = house4x4.getVoxels();
	}

	Vector3 pos;
	Vector3 offset = Vector3(start.x, y, start.y);

	for (Voxel v : voxels) {
		pos = v.getPosition();
		pos += offset;
		type = v.getType();
		chunkPos.x = pos.x;
		chunkPos.y = pos.z;
		chunkPos = fn::toChunkCoords(chunkPos);
		x = (int)chunkPos.x;
		z = (int)chunkPos.y;
		chunk = chunks[fn::fi2(x - xS, z - zS, C_W)];
		vx = (int)pos.x;
		vy = (int)pos.y;
		vz = (int)pos.z;
		chunk->setVoxel(
			vx % CHUNK_SIZE_X,
			vy % CHUNK_SIZE_Y,
			vz % CHUNK_SIZE_Z, type);
	}
}

vector<EcoGame::Area> EcoGame::findAreasOfSize(int size, int* mask, int* surfaceY, const int W, const int H) {
	int i, j, x, y, x1, x2, y1, y2, areaY, meanY = 0, count = 0;
	vector<EcoGame::Area> areas;

	int* sub = getIntBufferPool().borrow();

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
				
				if (sub[fn::fi2(i, j, W)] == size) {
					x1 = (i - size) + 1;
					y1 = (j - size) + 1;
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
