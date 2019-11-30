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
	Node* game = get_tree()->get_root()->get_node("EcoGame");
	ThreadPool::get()->submitTask(boost::bind(&EcoGame::buildAreasT, this, center, radius, minSideLength, game));
}

void EcoGame::buildAreasT(Vector2 center, float radius, float minSideLength, Node* game) {
	if (minSideLength < 1) return;
		
	int xS, xE, zS, zE, currentY, x, z, i, j, totalSquareArea;
	float nextArea;
	Area area;
	Vector2 start, end, offset;
	Array indices;
	Chunk* chunk;
	vector<Chunk*> chunks;
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

	//Godot::print(String("surfaceY and uniques set"));

	for (int nextY : uniques) {

		//Godot::print(String("nextY: {0}").format(Array::make(nextY)));

		// get the area covered by rectangles
		totalSquareArea = 0;
		for (i = 0; i < W * H; i++) {
			mask[i] = (surfaceY[i] == nextY) ? 1 : 0;
			totalSquareArea += (mask[i] > 0) ? 1 : 0;
		}

		nextArea = 0;

		// find all areas with surface area >= minSize
		while (true) {
			area = findNextArea(mask, nextY, W, H);
			start = area.getStart();
			end = area.getEnd();
			nextArea = (end.x - start.x) * (end.y - start.y);

			if (nextArea < MIN_SIZE) break;

			Godot::print(String("area start: {0}, end: {1}, y: {2}").format(Array::make(area.getStart(), area.getEnd(), area.getY())));

			offset.x = xS * CHUNK_SIZE_X;
			offset.y = zS * CHUNK_SIZE_Z;
			area.addOffset(offset);

			buildArea(area, chunks, xS, zS, C_W);
			markArea(area, mask, xS, zS, W);
		}
	}

	Variant v = game->call_deferred("update_chunks", indices, indices.size());

	getIntBufferPool().ret(surfaceY);
	getIntBufferPool().ret(mask);
}

void EcoGame::buildArea(EcoGame::Area area, vector<Chunk*> chunks, int xS, int zS, const int C_W) {
	int i, j, x, z, vx, vy, vz, y = (int) area.getY() + 1;
	Chunk* chunk;
	Vector2 start = area.getStart();
	Vector2 end = area.getEnd();
	Vector2 chunkPos;

	for (i = start.x; i < end.x; ++i) {
		for (j = start.y; j < end.y; ++j) {
			chunkPos.x = i;
			chunkPos.y = j;
			chunkPos = fn::toChunkCoords(chunkPos);
			x = (int)chunkPos.x;
			z = (int)chunkPos.y;
			chunk = chunks[fn::fi2(x - xS, z - zS, C_W)];
			chunk->setVoxel(
				i % CHUNK_SIZE_X, 
				y % CHUNK_SIZE_Y,
				j % CHUNK_SIZE_Z, 2);
		}
	}
}

EcoGame::Area EcoGame::findNextArea(int* mask, int y, const int W, const int H) {
	int i, j, x1, x2, y1, y2, area;
	int max_of_s, max_i, max_j;
	int* sub = getIntBufferPool().borrow();

	for (i = 0; i < W; i++)
		sub[fn::fi2(i, 0, W)] = mask[fn::fi2(i, 0, W)];

	for (j = 0; j < H; j++)
		sub[fn::fi2(0, j, W)] = mask[fn::fi2(0, j, W)];

	for (i = 1; i < W; i++) {
		for (j = 1; j < H; j++) {
			if (mask[fn::fi2(i, j, W)] == 1)
				sub[fn::fi2(i, j, W)] = min(
					sub[fn::fi2(i, j - 1, W)],
					min(sub[fn::fi2(i - 1, j, W)],
						sub[fn::fi2(i - 1, j - 1, W)])) + 1;
			else
				sub[fn::fi2(i, j, W)] = 0;
		}
	}

	max_of_s = sub[fn::fi2(0, 0, W)]; max_i = 0; max_j = 0;
	for (i = 0; i < W; i++) {
		for (j = 0; j < H; j++) {
			if (max_of_s < sub[fn::fi2(i, j, W)]) {
				max_of_s = sub[fn::fi2(i, j, W)];
				max_i = i;
				max_j = j;
			}
		}
	}

	x1 = max_i - max_of_s + 1;
	y1 = max_j - max_of_s + 1;
	x2 = max_i + 1;
	y2 = max_j + 1;

	getIntBufferPool().ret(sub);

	return EcoGame::Area(Vector2(x1, y1), Vector2(x2, y2), y);
}

void EcoGame::markArea(EcoGame::Area area, int* mask, int xS, int zS, const int W) {
	int x, z;
	Vector2 start = area.getStart();
	Vector2 end = area.getEnd();
	for (z = start.y; z <= end.y; ++z) {
		for (x = start.x; x <= end.x; ++x) {
			mask[fn::fi2(x - (xS * CHUNK_SIZE_X), z - (zS * CHUNK_SIZE_Z), W)] = 0;
		}
	}
}
