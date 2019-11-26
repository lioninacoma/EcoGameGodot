#include "ecogame.h"

using namespace godot;

void EcoGame::_register_methods() {
	register_method("buildChunk", &EcoGame::buildChunk);
	register_method("getSquares", &EcoGame::getSquares);
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

Array EcoGame::getSquares(Vector2 center, float radius, float minSideLength) {
	Array squares = Array();
	if (minSideLength < 1) return squares;

	Vector3 tl = Vector3(center.x - radius, 0, center.y - radius);
	Vector3 br = Vector3(center.x + radius, 0, center.y + radius);
	tl = fn::toChunkCoords(tl);
	br = fn::toChunkCoords(br);

	int xS = (int)tl.x;
	xS = max(0, xS);

	int xE = (int)br.x;
	xE = min(WORLD_SIZE, xE);

	int zS = (int)tl.z;
	zS = max(0, zS);

	int zE = (int)br.z;
	zE = min(WORLD_SIZE, zE);

	const float minSize = minSideLength * minSideLength;

	vector<int>::iterator it;
	vector<int> uniques;

	const int W = (xE - xS + 1) * CHUNK_SIZE_X;
	const int H = (zE - zS + 1) * CHUNK_SIZE_Z;
	int currentY, x, z, i, j, totalSquareArea;
	float nextArea;

	int* surfaceY = getIntBufferPool().borrow();
	int* mask = getIntBufferPool().borrow();

	//Godot::print(String("center: {0}, radius: {1}, tl: {2}, br: {3}, W: {4}, H: {5}").format(Array::make(center, radius, tl, br, W, H)));

	for (z = zS; z <= zE; z++) {
		for (x = xS; x <= xE; x++) {
			Chunk* chunk = getChunk(x, z);

			if (!chunk) continue;

			for (j = 0; j < CHUNK_SIZE_Z; j++) {
				for (i = 0; i < CHUNK_SIZE_X; i++) {
					currentY = chunk->getCurrentSurfaceY(i, j);
					uniques.push_back(currentY);
					surfaceY[fn::fi2((x - xS) * CHUNK_SIZE_X + i, (z - zS) * CHUNK_SIZE_Z + j, W)] = currentY;
					//cout << std::setfill(' ') << std::setw(2) << currentY;
				}
				//cout << endl;
			}
			//cout << endl;
		}
	}

	/*for (j = 0; j < H; j++) {
		for (i = 0; i < W; i++) {
			cout << std::setfill(' ') << std::setw(2) << surfaceY[fn::fi2(i, j, W)];
		}
		cout << endl;
	}
	cout << endl;

	getIntBufferPool().ret(surfaceY);
	getIntBufferPool().ret(mask);
	return squares;*/

	sort(uniques.begin(), uniques.end());
	it = unique(uniques.begin(), uniques.end());
	uniques.resize(distance(uniques.begin(), it));

	for (int nextY : uniques) {
		// get the area covered by rectangles
		totalSquareArea = 0;
		for (i = 0; i < W * H; i++) {
			mask[i] = (surfaceY[i] == nextY) ? 1 : 0;
			totalSquareArea += (mask[i] > 0) ? 1 : 0;
		}

		nextArea = 0;

		// find all rectangle until their area matches the total
		while (true) {
			PoolVector2Array square = findNextSquare(mask, W, H);
			nextArea = (square[1].x - square[0].x) * (square[1].y - square[0].y);

			if (nextArea >= minSize) {
				square.push_back(Vector2(nextY, nextArea));
				square.push_back(Vector2(xS * CHUNK_SIZE_X, zS * CHUNK_SIZE_Z));
				squares.push_back(square);
				markSquare(square, mask, W);
			}
			else {
				break;
			}
		}
	}

	getIntBufferPool().ret(surfaceY);
	getIntBufferPool().ret(mask);
	return squares;
}

PoolVector2Array EcoGame::findNextSquare(int* mask, const int W, const int H) {
	PoolVector2Array square;
	square.resize(2);

	PoolVector2Array::Write squareWrite = square.write();

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

	squareWrite[0] = Vector2(x1, y1);
	squareWrite[1] = Vector2(x2, y2);

	getIntBufferPool().ret(sub);

	return square;
}

void EcoGame::markSquare(PoolVector2Array rect, int* mask, const int W) {
	for (int i = rect[0].x; i <= rect[1].x; ++i) {
		for (int j = rect[0].y; j <= rect[1].y; ++j) {
			mask[fn::fi2(i, j, W)] = 0;
		}
	}
}

