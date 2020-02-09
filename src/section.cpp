#include "section.h"
#include "ecogame.h"

using namespace godot;

void Section::_register_methods() {
	register_method("getOffset", &Section::getOffset);
}

Section::Section(Vector2 offset, int sectionSize) {
	Section::offset = offset;
	Section::sectionSize = sectionSize;
	Section::sectionChunksLen = sectionSize * sectionSize;
	chunks = new boost::shared_ptr<Chunk>[sectionChunksLen * sizeof(*chunks)];

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

boost::shared_ptr<Chunk> Section::intersection(int x, int y, int z) {
	boost::shared_ptr<Chunk> chunk = getChunk(x - offset.x, z - offset.y);
	if (!chunk) return NULL;

	return chunk;
}

vector<boost::shared_ptr<Chunk>> Section::getChunksRay(Vector3 from, Vector3 to) {
	from = fn::toChunkCoords(from);
	to = fn::toChunkCoords(to);

	vector<boost::shared_ptr<Chunk>> list;
	boost::function<boost::shared_ptr<Chunk>(int, int, int)> intersection(boost::bind(&Section::intersection, this, _1, _2, _3));
	return Intersection::get<boost::shared_ptr<Chunk>>(from, to, false, intersection, list);
}

float Section::getVoxelAssetChance(int x, int y, float scale) {
	return noise->get_noise_2d(
		(x + offset.x * CHUNK_SIZE_X) * scale,
		(y + offset.y * CHUNK_SIZE_Z) * scale) / 2.0 + 0.5;
}

void Section::addVoxelAsset(Vector3 startV, VoxelAssetType type, boost::shared_ptr<ChunkBuilder> builder, Node* game) {
	VoxelAsset* voxelAsset = VoxelAssetManager::get()->getVoxelAsset(type);
	int i, areaSize = max(voxelAsset->getWidth(), voxelAsset->getHeight());
	boost::shared_ptr<Chunk> chunk;
	Vector2 start = Vector2(startV.x, startV.z);
	Vector2 end = start + Vector2(areaSize, areaSize);
	Area area = Area(start, end, startV.y);

	buildArea(area, type);

	for (i = 0; i < sectionChunksLen; i++) {
		chunk = getChunk(i);
		if (!chunk) continue;
		builder->build(chunk, game);
	}
}

Array Section::getDisconnectedVoxels(Vector3 position, Vector3 start, Vector3 end) {
	Array voxels;
	boost::shared_ptr<Chunk> chunk;
	int i, x, y, z, cx, cz;
	float nx, ny, nz, voxel;
	Vector3 point;
	Vector2 chunkOffset;
	boost::shared_ptr<GraphNode> current;
	unordered_map<size_t, boost::shared_ptr<GraphNode>> nodeCache;
	deque<size_t> queue, volume;
	unordered_set<size_t> inque, ready;
	vector<vector<boost::shared_ptr<GraphNode>>*> areas;
	vector<bool> areaOutOfBounds;
	int areaIndex = 0;
	size_t cHash, nHash;

	// von Neumann neighbourhood (3D)
	int neighbours[6][3] = {
		{ 1,  0,  0 }, 
		{ 0,  0,  1 }, 
		{-1,  0,  0 }, 
		{ 0,  0, -1 }, 
		{ 0,  1,  0 }, 
		{ 0, -1,  0 }
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
					current = boost::shared_ptr<GraphNode>(new GraphNode(Vector3(x, y, z), voxel));
					nodeCache.emplace(current->getHash(), current);
					volume.push_back(current->getHash());
				}
			}
		}
	}

	while (true) {
		auto area = new vector<boost::shared_ptr<GraphNode>>();
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

			if (point.x <= start.x || point.y <= start.y || point.z <= start.z
				|| point.x >= end.x || point.y >= end.y || point.z >= end.z) {
				areaOutOfBounds[areaIndex] = true;
			}

			for (i = 0; i < 6; i++) {
				x = neighbours[i][0];
				y = neighbours[i][1];
				z = neighbours[i][2];

				nx = point.x + x;
				ny = point.y + y;
				nz = point.z + z;

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
			auto voxel = Ref<Voxel>(Voxel::_new());
			voxel->setPosition(v->getPoint());
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
	boost::shared_ptr<Chunk> chunk;
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

bool Section::voxelAssetFits(Vector3 start, VoxelAssetType type) {
	VoxelAsset* voxelAsset = VoxelAssetManager::get()->getVoxelAsset(type);
	int maxDeltaY = voxelAsset->getMaxDeltaY();
	int areaSize = max(voxelAsset->getWidth(), voxelAsset->getHeight());
	int x, y, z, cx, cz, ci, dy;
	boost::shared_ptr<Chunk> chunk;
	Vector2 chunkOffset, end = Vector2(start.x, start.z) + Vector2(areaSize, areaSize);
	y = (int)start.y;

	for (z = start.z; z < end.y; z++) {
		for (x = start.x; x < end.x; x++) {
			chunkOffset.x = x;
			chunkOffset.y = z;
			chunkOffset = fn::toChunkCoords(chunkOffset);
			cx = (int)(chunkOffset.x - offset.x);
			cz = (int)(chunkOffset.y - offset.y);

			chunk = getChunk(cx, cz);

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

void Section::setVoxel(Vector3 position, int voxel, boost::shared_ptr<ChunkBuilder> builder, Node* game) {
	int x, y, z, cx, cz, ci;
	Vector2 chunkOffset;
	boost::shared_ptr<Chunk> chunk;
	chunkOffset.x = position.x;
	chunkOffset.y = position.z;
	chunkOffset = fn::toChunkCoords(chunkOffset);
	cx = (int)(chunkOffset.x - offset.x);
	cz = (int)(chunkOffset.y - offset.y);

	chunk = getChunk(cx, cz);

	if (!chunk) return;

	x = position.x;
	y = position.y;
	z = position.z;

	chunk->setVoxel(
		x % CHUNK_SIZE_X,
		y % CHUNK_SIZE_Y,
		z % CHUNK_SIZE_Z, voxel);
	builder->build(chunk, game);
}

int Section::getVoxel(Vector3 position) {
	int x, y, z, cx, cz, ci;
	Vector2 chunkOffset;
	boost::shared_ptr<Chunk> chunk;
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

boost::shared_ptr<GraphNode> Section::getNode(Vector3 position) {
	int x, y, z, cx, cz, ci;
	Vector2 chunkOffset;
	boost::shared_ptr<Chunk> chunk;
	boost::shared_ptr<GraphNode> node;
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

	node = chunk->getNode(fn::hash(position));
	if (node) return node;
	return chunk->findNode(position);
}

void Section::setChunk(int x, int z, boost::shared_ptr<Chunk> chunk) {
	int i = fn::fi2(x, z, sectionSize);
	return setChunk(i, chunk);
}

void Section::setChunk(int i, boost::shared_ptr<Chunk> chunk) {
	boost::unique_lock<boost::shared_timed_mutex> lock(CHUNKS_MUTEX);
	if (i < 0 || i >= sectionChunksLen) return;
	chunks[i] = chunk;
}

boost::shared_ptr<Chunk> Section::getChunk(int x, int z) {
	int i = fn::fi2(x, z, sectionSize);
	return getChunk(i);
}

boost::shared_ptr<Chunk> Section::getChunk(int i) {
	boost::shared_lock<boost::shared_timed_mutex> lock(CHUNKS_MUTEX);
	if (i < 0 || i >= sectionChunksLen) return NULL;
	return chunks[i];
}

void Section::fill(EcoGame* lib, int sectionSize) {
	int xS, xE, zS, zE, x, z, sx, sy, cx, cz, ci, si;
	boost::shared_ptr<Section> section;
	boost::shared_ptr<Chunk> chunk;
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

void Section::build(boost::shared_ptr<ChunkBuilder> builder, Node* game) {
	int x, y, i;
	boost::shared_ptr<Chunk> chunk;

	for (y = 0; y < sectionSize; y++) {
		for (x = 0; x < sectionSize; x++) {
			chunk = boost::shared_ptr<Chunk>(Chunk::_new());
			chunk->setOffset(Vector3((x + offset.x) * CHUNK_SIZE_X, 0, (y + offset.y) * CHUNK_SIZE_Z));
			setChunk(x, y, chunk);
			chunk->buildVolume();
		}
	}

	buildAreasByType(VoxelAssetType::PINE_TREE);
	//buildAreasByType(VoxelAssetType::HOUSE_6X6);
	//buildAreasByType(VoxelAssetType::HOUSE_4X4);


	for (i = 0; i < sectionChunksLen; i++) {
		chunk = getChunk(i);
		if (!chunk) continue;
		builder->build(chunk, game);
	}
}

Vector2 Section::getOffset() {
	boost::shared_lock<boost::shared_timed_mutex> lock(OFFSET_MUTEX);
	return Section::offset;
}

void Section::buildAreasByType(VoxelAssetType type) {
	VoxelAsset* voxelAsset = VoxelAssetManager::get()->getVoxelAsset(type);
	int maxDeltaY = voxelAsset->getMaxDeltaY();
	int areaSize = max(voxelAsset->getWidth(), voxelAsset->getHeight());
	float assetNoiseChance = voxelAsset->getNoiseChance();
	float assetNoiseOffset = voxelAsset->getNoiseOffset();
	float assetNoiseScale = voxelAsset->getNoiseScale();
	int currentY, currentType, currentIndex, deltaY, lastY = -1, ci, i, j, it, vx, vz;
	Vector2 areasOffset, start;
	Vector3 chunkOffset;
	boost::shared_ptr<Chunk> chunk;
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
		chunk = getChunk(it);

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

void Section::buildArea(Area area, VoxelAssetType type) {
	int voxelType, vx, vy, vz, ci, cx, cz, ay = area.getY() + 1;
	boost::shared_ptr<Chunk> chunk;
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

		chunk = getChunk(cx, cz);

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

vector<Area> Section::findAreasOfSize(int size, int* mask, int* surfaceY, int* sub) {
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