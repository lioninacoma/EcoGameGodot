#include "chunk.h"

using namespace godot;

void Chunk::_register_methods() {
	register_method("getOffset", &Chunk::getOffset);
	register_method("getVolume", &Chunk::getVolume);
	register_method("getVoxel", &Chunk::getVoxel);
	register_method("getMeshInstanceId", &Chunk::getMeshInstanceId);
	register_method("getVoxelRay", &Chunk::getVoxelRay);
	register_method("getCurrentSurfaceY", &Chunk::getCurrentSurfaceY);
	register_method("getFlatAreas", &Chunk::getFlatAreas);
	register_method("setOffset", &Chunk::setOffset);
	register_method("setVoxel", &Chunk::setVoxel);
	register_method("setMeshInstanceId", &Chunk::setMeshInstanceId);
	register_method("buildVolume", &Chunk::buildVolume);
}

Chunk::Chunk(Vector3 offset) {
	Chunk::offset = offset;
	Chunk::noise = OpenSimplexNoise::_new();
}

Chunk::~Chunk() {

}

void Chunk::_init() {
	Chunk::volume = new char[BUFFER_SIZE];
	Chunk::surfaceY = new int[CHUNK_SIZE_X * CHUNK_SIZE_Z];
	memset(volume, 0, BUFFER_SIZE * sizeof(*volume));
	memset(surfaceY, 0, CHUNK_SIZE_X * CHUNK_SIZE_Z * sizeof(*surfaceY));

	Chunk::noise->set_seed(NOISE_SEED);
	Chunk::noise->set_octaves(3);
	Chunk::noise->set_period(60.0);
	Chunk::noise->set_persistence(0.5);
}

int Chunk::getVoxel(int x, int y, int z) {
	if (x < 0 || x >= CHUNK_SIZE_X) return 0;
	if (y < 0 || y >= CHUNK_SIZE_Y) return 0;
	if (z < 0 || z >= CHUNK_SIZE_Z) return 0;
	return (int) volume[flattenIndex(x, y, z)];
}

int Chunk::getVoxelY(int x, int z) {
	float y = noise->get_noise_2d(
		(x + offset.x) * VOXEL_Y_NOISE_SCALE,
		(z + offset.z) * VOXEL_Y_NOISE_SCALE) / 2.0 + 0.5;
	y *= CHUNK_SIZE_Y;
	int yi = (int) y;

	for (int i = yi; i >= 0; i--) {
		float c = getVoxelChance(x, i, z);
		if (c < VOXEL_CHANCE_T) {
			return i;
		}
	}

	return 0;
}

int Chunk::flattenIndex(int x, int y, int z) {
	return x + CHUNK_SIZE_X * (y + CHUNK_SIZE_Y * z);
}

int Chunk::flattenIndex2D(int x, int z) {
	return x * CHUNK_SIZE_X + z;
}

float Chunk::getVoxelChance(int x, int y, int z) {
	return noise->get_noise_3d(
		(x + offset.x) * VOXEL_CHANCE_NOISE_SCALE,
		(y + offset.y) * VOXEL_CHANCE_NOISE_SCALE,
		(z + offset.z) * VOXEL_CHANCE_NOISE_SCALE) / 2.0 + 0.5;
}

Array Chunk::getFlatAreas(float minSideLength) {
	Array areas = Array();
	if (minSideLength < 1) return areas;
	
	const float minSize = minSideLength * minSideLength;
	const int W = CHUNK_SIZE_X;
	const int H = CHUNK_SIZE_Z;
	const int len = W * H;

	vector<int>::iterator it;
	vector<int> uniques;
	int mask[len];

	for (int i = 0; i < len; i++)
		uniques.push_back(surfaceY[i]);

	sort(uniques.begin(), uniques.end());
	it = unique(uniques.begin(), uniques.end());
	uniques.resize(distance(uniques.begin(), it));

	for (int next : uniques) {
		for (int i = 0; i < len; i++) {
			mask[i] = (surfaceY[i] == next) ? 1 : 0;
		}

		// get the area covered by rectangles
		int totalRectArea = 0;
		for (int i = 0; i < W; ++i) {
			for (int j = 0; j < H; ++j) {
				totalRectArea += mask[flattenIndex2D(i, j)] > 0 ? 1 : 0;
			}
		}

		float nextArea = 0;
		// find all rectangle until their area matches the total
		while (true) {
			PoolVector2Array rect = findNextRect(mask);
			nextArea = (rect[1].x - rect[0].x) * (rect[1].y - rect[0].y);

			if (nextArea >= minSize) {
				cout << nextArea << endl;
				areas.push_back(rect);
				markRect(rect, mask);
			}
			else {
				break;
			}
		}
	}
	
	return areas;
}

PoolVector2Array Chunk::findNextRect(int* mask) {
	const int W = CHUNK_SIZE_X;
	const int H = CHUNK_SIZE_Z;
	const int len = W * H;
	PoolVector2Array rect;

	rect.resize(2);
	PoolVector2Array::Write rectWrite = rect.write();

	int i, j;
	int max_of_s, max_i, max_j;
	int sub[len];

	/* Set first column of S[][]*/
	for (i = 0; i < W; i++)
		sub[flattenIndex2D(i, 0)] = mask[flattenIndex2D(i, 0)];

	/* Set first row of S[][]*/
	for (j = 0; j < H; j++)
		sub[flattenIndex2D(0, j)] = mask[flattenIndex2D(0, j)];

	/* Construct other entries of S[][]*/
	for (i = 1; i < W; i++) {
		for (j = 1; j < H; j++) {
			if (mask[flattenIndex2D(i, j)] == 1)
				sub[flattenIndex2D(i, j)] = min(
					sub[flattenIndex2D(i, j - 1)], 
					min(sub[flattenIndex2D(i - 1, j)], 
						sub[flattenIndex2D(i - 1, j - 1)])) + 1;
			else
				sub[flattenIndex2D(i, j)] = 0;
		}
	}

	/* Find the maximum entry, and indexes of maximum entry
		in S[][] */
	max_of_s = sub[flattenIndex2D(0, 0)]; max_i = 0; max_j = 0;
	for (i = 0; i < W; i++) {
		for (j = 0; j < H; j++) {
			if (max_of_s < sub[flattenIndex2D(i, j)]) {
				max_of_s = sub[flattenIndex2D(i, j)];
				max_i = i;
				max_j = j;
			}
		}
	}

	rectWrite[0] = Vector2(max_i - max_of_s + 1, max_j - max_of_s + 1);
	rectWrite[1] = Vector2(max_i, max_j);

	return rect;
}

void Chunk::markRect(PoolVector2Array rect, int* mask) {
	for (int i = rect[0].x; i <= rect[1].x; ++i) {
		for (int j = rect[0].y; j <= rect[1].y; ++j) {
			mask[flattenIndex2D(i, j)] = 0;
		}
	}
}

Voxel* Chunk::intersection(int x, int y, int z) {
	int chunkX = (int) offset.x;
	int chunkY = (int) offset.y;
	int chunkZ = (int) offset.z;
	if (x >= chunkX && x < chunkX + CHUNK_SIZE_X
		&& y >= chunkY && y < chunkY + CHUNK_SIZE_Y
		&& z >= chunkZ && z < chunkZ + CHUNK_SIZE_Z) {
		int v = getVoxel(
			(int) x % CHUNK_SIZE_X,
			(int) y % CHUNK_SIZE_Y,
			(int) z % CHUNK_SIZE_Z);

		if (v) {
			auto voxel = Voxel::_new();
			voxel->setPosition(Vector3(x, y, z));
			voxel->setChunkOffset(offset);
			voxel->setType(v);
			return voxel;
		}
	}

	return NULL;
}

int Chunk::getCurrentSurfaceY(int x, int z) {
	return surfaceY[flattenIndex2D(x, z)];
}

Ref<Voxel> Chunk::getVoxelRay(Vector3 from, Vector3 to) {
	vector<Voxel*> list;
	Ref<Voxel> voxel = NULL;

	boost::function<Voxel*(int, int, int)> intersection(boost::bind(&Chunk::intersection, this, _1, _2, _3));
	list = Intersection::get<Voxel*>(from, to, true, intersection, list);
	
	if (!list.empty()) {
		voxel = Ref<Voxel>(list.front());
	}

	return voxel;
}

void Chunk::setVoxel(int x, int y, int z, int v) {
	if (x < 0 || x >= CHUNK_SIZE_X) return;
	if (y < 0 || y >= CHUNK_SIZE_Y) return;
	if (z < 0 || z >= CHUNK_SIZE_Z) return;
	volume[flattenIndex(x, y, z)] = (char)v;

	int dy = surfaceY[flattenIndex2D(x, z)];

	if (v == 0 && dy == y) {
		for (int i = y - 1; i >= 0; i--) {
			surfaceY[flattenIndex2D(x, z)]--;
			if (getVoxel(x, i, z)) {
				break;
			}
		}
	}
	else {
		surfaceY[flattenIndex2D(x, z)] = max(dy, y);
	}
}

int Chunk::buildVolume() {
	int amountVoxel = 0;

	for (int z = 0; z < CHUNK_SIZE_Z; z++) {
		for (int x = 0; x < CHUNK_SIZE_X; x++) {
			int y = getVoxelY(x, z);
			for (int i = 0; i < y; i++) {
				float c = getVoxelChance(x, i, z);
				if (c < VOXEL_CHANCE_T) {
					setVoxel(x, i, z, 1);
					amountVoxel++;
				}
			}
		}
	}

	return amountVoxel;
}