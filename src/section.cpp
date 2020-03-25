#include "section.h"
#include "ecogame.h"
#include "graphnode.h"

using namespace godot;

void Section::_register_methods() {
	register_method("getOffset", &Section::getOffset);
}

Section::Section(Vector2 offset, int sectionSize) {
	Section::offset = offset;
	Section::sectionSize = sectionSize;
	Section::sectionChunksLen = sectionSize * sectionSize;
	chunks = new std::shared_ptr<Chunk>[sectionChunksLen * sizeof(*chunks)];

	memset(chunks, 0, sectionChunksLen * sizeof(*chunks));

	Section::noise = OpenSimplexNoise::_new();
	Section::noise->set_seed(NOISE_SEED);
	Section::noise->set_octaves(3);
	Section::noise->set_period(60.0);
	Section::noise->set_persistence(0.5);
};

Section::~Section() {
	delete[] chunks;
}

std::shared_ptr<Chunk> Section::intersection(int x, int y, int z) {
	std::shared_ptr<Chunk> chunk = getChunk(x - offset.x, z - offset.y);
	if (!chunk) return NULL;

	return chunk;
}

vector<std::shared_ptr<Chunk>> Section::getChunksRay(Vector3 from, Vector3 to) {
	from = fn::toChunkCoords(from);
	to = fn::toChunkCoords(to);

	vector<std::shared_ptr<Chunk>> list;
	boost::function<std::shared_ptr<Chunk>(int, int, int)> intersection(boost::bind(&Section::intersection, this, _1, _2, _3));
	return Intersection::get<std::shared_ptr<Chunk>>(from, to, false, intersection, list);
}

Array Section::getDisconnectedVoxels(Vector3 position, Vector3 start, Vector3 end) {
	Array voxels;
	std::shared_ptr<Chunk> chunk;
	int i, x, y, z, cx, cz;
	float nx, ny, nz, voxel;
	Vector2 chunkOffset;
	std::shared_ptr<Vector3> point;
	std::shared_ptr<GraphNavNode> current;
	unordered_map<size_t, std::shared_ptr<GraphNavNode>> nodeCache;
	deque<size_t> queue, volume;
	unordered_set<size_t> inque, ready;
	vector<vector<std::shared_ptr<GraphNavNode>>*> areas;
	vector<bool> areaOutOfBounds;
	int areaIndex = 0;
	size_t cHash, nHash;

	// von Neumann neighbourhood (3D)
	int neighbours[6][3] = {
		{ 0,  1,  0 },
		{ 0, -1,  0 },
		{-1,  0,  0 },
		{ 1,  0,  0 },
		{ 0,  0,  1 },
		{ 0,  0, -1 }
	};

	for (z = start.z; z <= end.z; z++) {
		for (x = start.x; x <= end.x; x++) {
			chunkOffset.x = x;
			chunkOffset.y = z;
			chunkOffset = fn::toChunkCoords(chunkOffset);
			cx = (int)(chunkOffset.x - offset.x);
			cz = (int)(chunkOffset.y - offset.y);

			chunk = getChunk(cx, cz);

			if (!chunk) continue;

			for (y = start.y; y <= end.y; y++) {
				voxel = chunk->getVoxel(
					x % CHUNK_SIZE_X,
					y % CHUNK_SIZE_Y,
					z % CHUNK_SIZE_Z);
				if (voxel) {
					GraphNavNode* node = GraphNavNode::_new();
					node->setPoint(Vector3(x, y, z));
					node->setVoxel(voxel);
					current = std::shared_ptr<GraphNavNode>(node);
					nodeCache.emplace(current->getHash(), current);
					volume.push_back(current->getHash());
				}
			}
		}
	}

	while (true) {
		auto area = new vector<std::shared_ptr<GraphNavNode>>();
		areaOutOfBounds.push_back(false);

		while (!volume.empty()) {
			cHash = volume.front();
			volume.pop_front();
			if (ready.find(cHash) == ready.end()) {
				queue.push_back(cHash);
				inque.insert(cHash);
				break;
			}
		}
		if (queue.empty()) {
			break;
		}

		while (!queue.empty()) {
			cHash = queue.front();
			queue.pop_front();
			inque.erase(cHash);

			current = nodeCache[cHash];
			area->push_back(current);
			ready.insert(cHash);
			point = current->getPoint();

			if (point->x <= start.x || point->y <= start.y || point->z <= start.z
				|| point->x >= end.x || point->y >= end.y || point->z >= end.z) {
				areaOutOfBounds[areaIndex] = true;
			}

			for (i = 0; i < 6; i++) {
				x = neighbours[i][0];
				y = neighbours[i][1];
				z = neighbours[i][2];

				nx = point->x + x;
				ny = point->y + y;
				nz = point->z + z;

				nHash = fn::hash(Vector3(nx, ny, nz));

				if (nodeCache.find(nHash) == nodeCache.end()) continue;
				if (inque.find(nHash) == inque.end() && ready.find(nHash) == ready.end()) {
					queue.push_back(nHash);
					inque.insert(nHash);
				}
			}
		}

		areas.push_back(area);

		if (ready.size() >= nodeCache.size()) {
			break;
		}

		areaIndex++;
	}

	areaIndex = -1;
	for (bool b : areaOutOfBounds) {
		areaIndex++;
		if (b) continue;
		for (auto v : *areas[areaIndex]) {
			auto voxel = Voxel::_new();
			voxel->setPosition(v->getPointU());
			voxel->setType(v->getVoxel());
			voxels.push_back(voxel);
		}
	}

	for (auto area : areas) {
		for (auto v : *areas[areaIndex])
			v.reset();
		area->clear();
		delete area;
	}
	nodeCache.clear();

	return voxels;
}

PoolVector3Array Section::findVoxelsInRange(Vector3 startV, float radius, int voxel) {
	PoolVector3Array voxels;

	int x, y, z, cx, cz, ci, dy;
	std::shared_ptr<Chunk> chunk;
	Vector2 chunkOffset;
	Vector3 start = startV - Vector3(radius, radius, radius);
	Vector3 end = startV + Vector3(radius, radius, radius);

	for (z = start.z; z < end.z; z++) {
		for (x = start.x; x < end.x; x++) {
			chunkOffset.x = x;
			chunkOffset.y = z;
			chunkOffset = fn::toChunkCoords(chunkOffset);
			cx = (int)(chunkOffset.x - offset.x);
			cz = (int)(chunkOffset.y - offset.y);

			chunk = getChunk(cx, cz);

			if (!chunk) continue;

			for (y = start.y; y < end.y; y++) {
				if (chunk->getVoxel(
					x % CHUNK_SIZE_X,
					y % CHUNK_SIZE_Y,
					z % CHUNK_SIZE_Z) == voxel) {
					voxels.push_back(Vector3(x, y, z));
				}
			}
		}
	}

	return voxels;
}

//void Section::setVoxel(Vector3 position, float voxel) {
//	int x, y, z, cx, cz, ci;
//	Vector2 chunkOffset;
//	std::shared_ptr<Chunk> chunk;
//	chunkOffset.x = position.x;
//	chunkOffset.y = position.z;
//	chunkOffset = fn::toChunkCoords(chunkOffset);
//	cx = (int)(chunkOffset.x - offset.x);
//	cz = (int)(chunkOffset.y - offset.y);
//
//	chunk = getChunk(cx, cz);
//
//	if (!chunk) return;
//
//	x = position.x;
//	y = position.y;
//	z = position.z;
//
//	chunk->setVoxel(
//		x % CHUNK_SIZE_X,
//		y % CHUNK_SIZE_Y,
//		z % CHUNK_SIZE_Z, voxel);
//
//	int i, j;
//	auto lib = EcoGame::get();
//	for (j = -1; j < 2; j++)
//		for (i = -1; i < 2; i++) {
//			chunk = lib->getChunk(Vector3(chunkOffset.x * CHUNK_SIZE_X + i * CHUNK_SIZE_X, 0, chunkOffset.y * CHUNK_SIZE_Z + j * CHUNK_SIZE_Z));
//			if (chunk) builder->build(chunk);
//		}
//}

int Section::getVoxel(Vector3 position) {
	int x, y, z, cx, cz, ci;
	Vector2 chunkOffset;
	std::shared_ptr<Chunk> chunk;
	chunkOffset.x = position.x;
	chunkOffset.y = position.z;
	chunkOffset = fn::toChunkCoords(chunkOffset);
	cx = (int)(chunkOffset.x - offset.x);
	cz = (int)(chunkOffset.y - offset.y);

	chunk = getChunk(cx, cz);

	if (!chunk) return 0;

	x = position.x;
	y = position.y;
	z = position.z;

	return chunk->getVoxel(
		x % CHUNK_SIZE_X,
		y % CHUNK_SIZE_Y,
		z % CHUNK_SIZE_Z);
}

std::shared_ptr<GraphNavNode> Section::getNode(Vector3 position) {
	int x, y, z, cx, cz, ci;
	Vector2 chunkOffset;
	std::shared_ptr<Chunk> chunk;
	std::shared_ptr<GraphNavNode> node;
	chunkOffset.x = position.x;
	chunkOffset.y = position.z;
	chunkOffset = fn::toChunkCoords(chunkOffset);
	cx = (int)(chunkOffset.x - offset.x);
	cz = (int)(chunkOffset.y - offset.y);

	chunk = getChunk(cx, cz);

	if (!chunk) return NULL;

	x = position.x;
	y = position.y;
	z = position.z;

	return chunk->findNode(position);
}

void Section::setChunk(int x, int z, std::shared_ptr<Chunk> chunk) {
	int i = fn::fi2(x, z, sectionSize);
	return setChunk(i, chunk);
}

void Section::setChunk(int i, std::shared_ptr<Chunk> chunk) {
	boost::unique_lock<boost::mutex> lock(CHUNKS_MUTEX);
	if (i < 0 || i >= sectionChunksLen) return;
	chunks[i] = chunk;
}

std::shared_ptr<Chunk> Section::getChunk(int x, int z) {
	int i = fn::fi2(x, z, sectionSize);
	return getChunk(i);
}

std::shared_ptr<Chunk> Section::getChunk(int i) {
	boost::unique_lock<boost::mutex> lock(CHUNKS_MUTEX);
	if (i < 0 || i >= sectionChunksLen) return NULL;
	return chunks[i];
}

void Section::build(std::shared_ptr<ChunkBuilder> builder) {
	int x, y, i;
	std::shared_ptr<Chunk> chunk;

	float world_width_2 = CHUNK_SIZE_X * WORLD_SIZE / 2;
	float world_height_2 = CHUNK_SIZE_Y / 2;
	float world_depth_2 = CHUNK_SIZE_Z * WORLD_SIZE / 2;
	Vector3 cog = Vector3(world_width_2, world_height_2, world_depth_2);

	for (y = 0; y < sectionSize; y++) {
		for (x = 0; x < sectionSize; x++) {
			chunk = std::shared_ptr<Chunk>(Chunk::_new());
			chunk->setOffset(Vector3((x + offset.x) * CHUNK_SIZE_X, 0, (y + offset.y) * CHUNK_SIZE_Z));
			chunk->setSection(std::shared_ptr<Section>(this));
			chunk->setCenterOfGravity(cog);
			setChunk(x, y, chunk);
			chunk->buildVolume();
		}
	}

	for (i = 0; i < sectionChunksLen; i++) {
		chunk = getChunk(i);
		if (!chunk) continue;
		builder->build(chunk);
	}
}

Vector2 Section::getOffset() {
	return Section::offset;
}

void Section::fill(EcoGame* lib, int sectionSize) {
	int xS, xE, zS, zE, x, z, sx, sy, cx, cz, ci, si;
	std::shared_ptr<Section> section;
	std::shared_ptr<Chunk> chunk;
	Vector3 tl, br, chunkCoords;

	tl = Vector3(offset.x, 0, offset.y);
	br = tl + Vector3(Section::sectionSize, 0, Section::sectionSize);

	xS = int(tl.x);
	xS = max(0, xS);

	xE = int(br.x);
	xE = min(WORLD_SIZE, xE);

	zS = int(tl.z);
	zS = max(0, zS);

	zE = int(br.z);
	zE = min(WORLD_SIZE, zE);

	for (z = (int)(zS / sectionSize); z <= (int)(zE / sectionSize); z++) {
		for (x = (int)(xS / sectionSize); x <= (int)(xE / sectionSize); x++) {
			section = lib->getSection(x, z);
			if (!section) continue;

			for (sy = 0; sy < sectionSize; sy++) {
				for (sx = 0; sx < sectionSize; sx++) {
					chunk = section->getChunk(sx, sy);
					if (!chunk) continue;

					chunkCoords = chunk->getOffset();
					chunkCoords = fn::toChunkCoords(chunkCoords);

					if (chunkCoords.x >= xS && chunkCoords.z >= zS && chunkCoords.x < xE && chunkCoords.z < zE) {
						cx = (int)(chunkCoords.x - offset.x);
						cz = (int)(chunkCoords.z - offset.y);
						setChunk(cx, cz, chunk);
					}
				}
			}
		}
	}
}