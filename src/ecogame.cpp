#include "ecogame.h"
#include "navigator.h"

using namespace godot;

void EcoGame::_register_methods() {
	register_method("buildChunk", &EcoGame::buildChunk);
	register_method("buildVoxelAsset", &EcoGame::buildVoxelAsset);
	register_method("addVoxelAsset", &EcoGame::addVoxelAsset);
	register_method("voxelAssetFits", &EcoGame::voxelAssetFits);
	register_method("setVoxel", &EcoGame::setVoxel);
	register_method("getVoxel", &EcoGame::getVoxel);
	register_method("getVoxelsInArea", &EcoGame::getVoxelsInArea);
	register_method("getDisconnectedVoxels", &EcoGame::getDisconnectedVoxels);
	register_method("buildSections", &EcoGame::buildSections);
	register_method("navigate", &EcoGame::navigate);
	register_method("navigateToClosestVoxel", &EcoGame::navigateToClosestVoxel);
	register_method("findVoxelsInRange", &EcoGame::findVoxelsInRange);
	register_method("updateGraph", &EcoGame::updateGraph);
	register_method("updateGraphs", &EcoGame::updateGraphs);
}

EcoGame::EcoGame() {
	chunkBuilder = boost::shared_ptr<ChunkBuilder>(new ChunkBuilder());
	sections = new boost::shared_ptr<Section>[SECTIONS_LEN * sizeof(*sections)];

	memset(sections, 0, SECTIONS_LEN * sizeof(*sections));
}

EcoGame::~EcoGame() {
	delete[] sections;
	chunkBuilder.reset();
}

void EcoGame::_init() {
	// initialize any variables here
}

PoolVector3Array EcoGame::getDisconnectedVoxels(Vector3 center, float radius) {
	boost::shared_ptr<Section> section;

	Vector3 start = center - Vector3(radius, radius, radius);
	Vector3 end = center + Vector3(radius, radius, radius);
	Vector3 tl = start;
	Vector3 br = end;

	tl = fn::toChunkCoords(tl);
	br = fn::toChunkCoords(br);

	Vector2 offset = Vector2(tl.x, tl.z);
	int sectionSize = max(abs(br.x - tl.x), abs(br.z - tl.z)) + 1;

	section = boost::shared_ptr<Section>(new Section(offset, sectionSize));
	section->fill(this, SECTION_SIZE);
	PoolVector3Array voxels = section->getDisconnectedVoxels(start, end);
	section.reset();
	return voxels;
}

PoolVector3Array EcoGame::getVoxelsInArea(Vector3 start, Vector3 end, int voxel) {
	int i, j;
	PoolVector3Array currentVoxels, result;
	unordered_map<size_t, Vector3> voxels;
	boost::shared_ptr<Section> section;
	Vector3 tl = start;
	Vector3 br = end;
	Vector3 current;
	Vector2 start2 = Vector2(start.x, start.z);
	Vector2 end2 = Vector2(end.x, end.z);

	tl = fn::toChunkCoords(tl);
	br = fn::toChunkCoords(br);

	Vector2 offset = Vector2(tl.x, tl.z);
	int sectionSize = max(abs(br.x - tl.x), abs(br.z - tl.z)) + 1;
	int sectionChunksLen = sectionSize * sectionSize;

	section = boost::shared_ptr<Section>(new Section(offset, sectionSize));
	section->fill(this, SECTION_SIZE);

	for (i = 0; i < sectionChunksLen; i++) {
		auto chunk = section->getChunk(i);
		if (!chunk) continue;

		std::function<void(std::pair<size_t, boost::shared_ptr<GraphNode>>)> nodeFn = [&](auto next) {
			currentVoxels = chunk->getReachableVoxelsOfType(next.second->getPoint(), voxel);
			
			for (j = 0; j < currentVoxels.size(); j++) {
				current = currentVoxels[j];
				if (!Intersection::isPointInRectangle(start2, end2, Vector2(current.x + 0.5, current.z + 0.5))) continue;
				voxels.emplace(fn::hash(current), current);
			}
		};
		chunk->forEachNode(nodeFn);
	}

	section.reset();

	for (auto v : voxels)
		result.push_back(v.second);

	return result;
}

boost::shared_ptr<Section> EcoGame::intersection(int x, int y, int z) {
	//Godot::print(String("Section: {0}").format(Array::make(Vector3(x, y, z))));
	boost::shared_ptr<Section> section = getSection(x, z);
	if (!section) return NULL;

	return section;
}

vector<boost::shared_ptr<Section>> EcoGame::getSectionsRay(Vector3 from, Vector3 to) {
	from = fn::toSectionCoords(from);
	to = fn::toSectionCoords(to);

	vector<boost::shared_ptr<Section>> list;
	boost::function<boost::shared_ptr<Section>(int, int, int)> intersection(boost::bind(&EcoGame::intersection, this, _1, _2, _3));
	return Intersection::get<boost::shared_ptr<Section>>(from, to, false, intersection, list);
}

vector<boost::shared_ptr<Chunk>> EcoGame::getChunksRay(Vector3 from, Vector3 to) {
	vector<boost::shared_ptr<Chunk>> list;
	vector<boost::shared_ptr<Chunk>> chunks;
	vector<boost::shared_ptr<Section>> sections = getSectionsRay(from, to);
	
	for (auto section : sections) {
		chunks = section->getChunksRay(from, to);
		list.insert(list.end(), chunks.begin(), chunks.end());
	}
	return list;
}

vector<boost::shared_ptr<Chunk>> EcoGame::getChunksInRange(Vector3 center, float radius) {
	vector<boost::shared_ptr<Chunk>> list;
	int x, z, i;
	int dist = radius / (SECTION_SIZE * CHUNK_SIZE_X);
	Vector3 p = Vector3(center.x, 0, center.z);
	Vector3 cc = Vector3(CHUNK_SIZE_X / 2, 0, CHUNK_SIZE_Z / 2);
	boost::shared_ptr<Section> section;

	p = fn::toSectionCoords(p);

	for (z = -dist + (int)p.z; z <= dist + (int)p.z; z++) {
		for (x = -dist + (int)p.x; x <= dist + (int)p.x; x++) {
			if (x < 0 || z < 0 || x >= SECTIONS_SIZE || z >= SECTIONS_SIZE) continue;
			section = getSection(x, z);

			if (!section) continue;

			for (i = 0; i < SECTION_CHUNKS_LEN; i++) {
				auto chunk = section->getChunk(i);

				if (!chunk || (chunk->getOffset() + cc).distance_to(center) > radius) continue;
				list.push_back(section->getChunk(i));
			}
		}
	}

	return list;
}

void EcoGame::navigate(Vector3 startV, Vector3 goalV, int actorInstanceId) {
	Node* game = get_tree()->get_root()->get_node("EcoGame");
	ThreadPool::getNav()->submitTask(boost::bind(&EcoGame::navigateTask, this, startV, goalV, actorInstanceId, game));
}

void EcoGame::navigateToClosestVoxel(Vector3 startV, int voxel, int actorInstanceId) {
	Node* game = get_tree()->get_root()->get_node("EcoGame");
	ThreadPool::getNav()->submitTask(boost::bind(&EcoGame::navigateToClosestVoxelTask, this, startV, voxel, actorInstanceId, game, this));
}

void EcoGame::navigateTask(Vector3 startV, Vector3 goalV, int actorInstanceId, Node* game) {
	Navigator::get()->navigate(startV, goalV, actorInstanceId, game, this);
}

void EcoGame::navigateToClosestVoxelTask(Vector3 startV, int voxel, int actorInstanceId, Node* game, EcoGame* lib) {
	Navigator::get()->navigateToClosestVoxel(startV, voxel, actorInstanceId, game, this);
}

void EcoGame::setVoxel(Vector3 position, int voxel) {
	Node* game = get_tree()->get_root()->get_node("EcoGame");
	Vector3 p = position;
	boost::shared_ptr<Section> section;
	p = fn::toSectionCoords(p);
	section = getSection(p.x, p.z);
	if (!section) return;
	section->setVoxel(position, voxel, chunkBuilder, game);
}

int EcoGame::getVoxel(Vector3 position) {
	Vector3 p = position;
	boost::shared_ptr<Section> section;
	p = fn::toSectionCoords(p);
	section = getSection(p.x, p.z);
	if (!section) return 0;
	return section->getVoxel(position);
}

boost::shared_ptr<GraphNode> EcoGame::getNode(Vector3 position) {
	Vector3 p = position;
	boost::shared_ptr<Section> section;
	p = fn::toSectionCoords(p);
	section = getSection(p.x, p.z);
	if (!section) return NULL;
	return section->getNode(position);
}

void EcoGame::updateGraph(Variant vChunk) {
	Node* game = get_tree()->get_root()->get_node("EcoGame");
	boost::shared_ptr<Chunk> chunk = boost::shared_ptr<Chunk>(as<Chunk>(vChunk.operator Object * ()));

	ThreadPool::get()->submitTask(boost::bind(&EcoGame::updateGraphTask, this, chunk, game));
}

void EcoGame::updateGraphTask(boost::shared_ptr<Chunk> chunk, Node* game) {
	Navigator::get()->updateGraph(chunk, game);
}

void EcoGame::setSection(int x, int z, boost::shared_ptr<Section> section) {
	int i = fn::fi2(x, z, SECTIONS_SIZE);
	return setSection(i, section);
}

void EcoGame::setSection(int i, boost::shared_ptr<Section> section) {
	boost::unique_lock<boost::shared_timed_mutex> lock(SECTIONS_MUTEX);
	if (i < 0 || i >= SECTIONS_LEN) return;
	sections[i] = section;
}

boost::shared_ptr<Section> EcoGame::getSection(int x, int z) {
	int i = fn::fi2(x, z, SECTIONS_SIZE);
	return getSection(i);
}

boost::shared_ptr<Section> EcoGame::getSection(int i) {
	boost::shared_lock<boost::shared_timed_mutex> lock(SECTIONS_MUTEX);
	if (i < 0 || i >= SECTIONS_LEN) return NULL;
	return sections[i];
}

void EcoGame::updateGraphs(Vector3 center, float radius) {
	Node* game = get_tree()->get_root()->get_node("EcoGame");
	int x, z, i;
	int dist = radius / (SECTION_SIZE * CHUNK_SIZE_X);
	Vector3 p = Vector3(center.x, 0, center.z);
	boost::shared_ptr<Section> section;
	
	p = fn::toSectionCoords(p);

	for (z = -dist + (int)p.z; z <= dist + (int)p.z; z++) {
		for (x = -dist + (int)p.x; x <= dist + (int)p.x; x++) {
			if (x < 0 || z < 0 || x >= SECTIONS_SIZE || z >= SECTIONS_SIZE) continue;
			if (p.distance_to(Vector3(x, 0, z)) > dist + 1) continue;

			section = getSection(x, z);

			if (!section) continue;
			
			for (i = 0; i < SECTION_CHUNKS_LEN; i++) {
				ThreadPool::get()->submitTask(boost::bind(&EcoGame::updateGraphTask, this, section->getChunk(i), game));
			}
		}
	}
}

void EcoGame::buildSections(Vector3 pos, float d, int maxSectionsBuilt) {
	int x, z;
	int dist = 0;
	int sectionsBuilt = 0;
	int it = 0;
	int maxIt = (int)((Math_PI * d * d) / (CHUNK_SIZE_X * SECTION_CHUNKS_LEN));
	Node* game = get_tree()->get_root()->get_node("EcoGame");
	boost::shared_ptr<Section> section;
	Vector3 p = Vector3(pos.x, 0, pos.z);

	p = fn::toSectionCoords(p);
	//Godot::print(String("maxIt: {0}, maxSectionsBuilt: {1})").format(Array::make(maxIt, maxSectionsBuilt)));

	while (sectionsBuilt < maxSectionsBuilt) {
		for (z = -dist + (int)p.z; z <= dist + (int)p.z; z++) {
			for (x = -dist + (int)p.x; x <= dist + (int)p.x; x++) {
				if (++it >= maxIt) return;
				if (x < 0 || z < 0 || x >= SECTIONS_SIZE || z >= SECTIONS_SIZE) continue;
				if (p.distance_to(Vector3(x, 0, z)) > dist + 1) continue;
				if (sections[fn::fi2(x, z, SECTIONS_SIZE)]) continue;

				Godot::print(String("section {0} building ...").format(Array::make(Vector2(x * SECTION_SIZE, z * SECTION_SIZE))));
				section = boost::shared_ptr<Section>(new Section(Vector2(x * SECTION_SIZE, z * SECTION_SIZE)));
				setSection(x, z, section);
				ThreadPool::get()->submitTask(boost::bind(&EcoGame::buildSection, this, section, game));

				if (++sectionsBuilt >= maxSectionsBuilt) return;
			}
		}
		dist++;
	}
}

void EcoGame::buildSection(boost::shared_ptr<Section> section, Node* game) {
	section->build(chunkBuilder, game);
}

void EcoGame::buildChunk(Variant vChunk) {
	boost::shared_ptr<Chunk> chunk = boost::shared_ptr<Chunk>(as<Chunk>(vChunk.operator Object * ()));
	Node* game = get_tree()->get_root()->get_node("EcoGame");
	chunkBuilder->build(chunk, game);
}

PoolVector3Array EcoGame::findVoxelsInRange(Vector3 startV, float radius, int voxel) {
	boost::shared_ptr<Section> section;

	Vector3 start = startV - Vector3(radius, radius, radius);
	Vector3 end = startV + Vector3(radius, radius, radius);
	Vector3 tl = start;
	Vector3 br = end;

	tl = fn::toChunkCoords(tl);
	br = fn::toChunkCoords(br);

	Vector2 offset = Vector2(tl.x, tl.z);
	int sectionSize = max(abs(br.x - tl.x), abs(br.z - tl.z)) + 1;

	section = boost::shared_ptr<Section>(new Section(offset, sectionSize));
	section->fill(this, SECTION_SIZE);
	PoolVector3Array voxels = section->findVoxelsInRange(startV, radius, voxel);
	section.reset();
	return voxels;
}

bool EcoGame::voxelAssetFits(Vector3 startV, int type) {
	VoxelAssetType vat = (VoxelAssetType)type;
	VoxelAsset* asset = VoxelAssetManager::get()->getVoxelAsset(vat);
	float width = asset->getWidth();
	float depth = asset->getDepth();
	boost::shared_ptr<Section> section;
	int w2 = (int)(width / 2.0);
	int d2 = (int)(depth / 2.0);

	Vector3 start = startV - Vector3(w2, 0, d2);
	Vector3 end = startV + Vector3(w2, 0, d2);
	Vector3 tl = start;
	Vector3 br = end;

	tl = fn::toChunkCoords(tl);
	br = fn::toChunkCoords(br);

	Vector2 offset = Vector2(tl.x, tl.z);
	int sectionSize = max(abs(br.x - tl.x), abs(br.z - tl.z)) + 1;

	section = boost::shared_ptr<Section>(new Section(offset, sectionSize));
	section->fill(this, SECTION_SIZE);
	bool fits = section->voxelAssetFits(start, vat);
	section.reset();
	return fits;
}

void EcoGame::addVoxelAsset(Vector3 startV, int type) {
	VoxelAssetType vat = (VoxelAssetType)type;
	VoxelAsset* asset = VoxelAssetManager::get()->getVoxelAsset(vat);
	Node* game = get_tree()->get_root()->get_node("EcoGame");
	float width = asset->getWidth();
	float depth = asset->getDepth();
	boost::shared_ptr<Section> section;
	int w2 = (int)(width / 2.0);
	int d2 = (int)(depth / 2.0);

	Vector3 start = startV - Vector3(w2, 0, d2);
	Vector3 end = startV + Vector3(w2, 0, d2);
	Vector3 tl = start;
	Vector3 br = end;

	tl = fn::toChunkCoords(tl);
	br = fn::toChunkCoords(br);

	Vector2 offset = Vector2(tl.x, tl.z);
	int sectionSize = max(abs(br.x - tl.x), abs(br.z - tl.z)) + 1;

	section = boost::shared_ptr<Section>(new Section(offset, sectionSize));
	section->fill(this, SECTION_SIZE);
	section->addVoxelAsset(start, vat, chunkBuilder, game);
	section.reset();
}

Array EcoGame::buildVoxelAsset(int type) {
	MeshBuilder meshBuilder;
	VoxelAssetType vat = (VoxelAssetType)type;
	VoxelAsset* asset = VoxelAssetManager::get()->getVoxelAsset(vat);

	const int BUFFER = asset->getWidth() * asset->getHeight() * asset->getDepth();
	const int MAX_VERTICES = (BUFFER * VERTEX_SIZE * 6 * 4) / 2;
	
	int i, j, o, n, offset, amountVertices, amountIndices;
	Array meshes;
	float* vertices;
	float** buffers = new float*[TYPES];

	for (i = 0; i < TYPES; i++) {
		buffers[i] = new float[MAX_VERTICES];
		memset(buffers[i], 0, MAX_VERTICES * sizeof(*buffers[i]));
	}

	vector<int> offsets = meshBuilder.buildVertices(vat, buffers, TYPES);

	for (o = 0; o < offsets.size(); o++) {
		offset = offsets[o];
		if (offset <= 0) continue;

		vertices = buffers[o];
		amountVertices = offset / VERTEX_SIZE;
		amountIndices = amountVertices / 2 * 3;

		Array meshData;
		Array meshArrays;
		PoolVector3Array vertexArray;
		PoolVector3Array normalArray;
		PoolVector2Array uvArray;
		PoolIntArray indexArray;
		PoolVector3Array collisionArray;

		meshArrays.resize(Mesh::ARRAY_MAX);
		vertexArray.resize(amountVertices);
		normalArray.resize(amountVertices);
		uvArray.resize(amountVertices);
		indexArray.resize(amountIndices);
		collisionArray.resize(amountIndices);

		PoolVector3Array::Write vertexArrayWrite = vertexArray.write();
		PoolVector3Array::Write normalArrayWrite = normalArray.write();
		PoolVector3Array::Write collisionArrayWrite = collisionArray.write();
		PoolVector2Array::Write uvArrayWrite = uvArray.write();
		PoolIntArray::Write indexArrayWrite = indexArray.write();

		for (i = 0, n = 0; i < offset; i += VERTEX_SIZE, n++) {
			vertexArrayWrite[n] = Vector3(vertices[i], vertices[i + 1], vertices[i + 2]);
			normalArrayWrite[n] = Vector3(vertices[i + 3], vertices[i + 4], vertices[i + 5]);
			uvArrayWrite[n] = Vector2(vertices[i + 6], vertices[i + 7]);
		}

		for (i = 0, j = 0; i < amountIndices; i += 6, j += 4) {
			indexArrayWrite[i + 0] = j + 2;
			indexArrayWrite[i + 1] = j + 1;
			indexArrayWrite[i + 2] = j;
			indexArrayWrite[i + 3] = j;
			indexArrayWrite[i + 4] = j + 3;
			indexArrayWrite[i + 5] = j + 2;
			collisionArrayWrite[i + 0] = vertexArrayWrite[j + 2];
			collisionArrayWrite[i + 1] = vertexArrayWrite[j + 1];
			collisionArrayWrite[i + 2] = vertexArrayWrite[j];
			collisionArrayWrite[i + 3] = vertexArrayWrite[j];
			collisionArrayWrite[i + 4] = vertexArrayWrite[j + 3];
			collisionArrayWrite[i + 5] = vertexArrayWrite[j + 2];
		}

		meshArrays[Mesh::ARRAY_VERTEX] = vertexArray;
		meshArrays[Mesh::ARRAY_NORMAL] = normalArray;
		meshArrays[Mesh::ARRAY_TEX_UV] = uvArray;
		meshArrays[Mesh::ARRAY_INDEX] = indexArray;

		meshData.push_back(meshArrays);
		meshData.push_back(collisionArray);
		meshData.push_back(o + 1);
		meshData.push_back(offset);

		meshes.push_back(meshData);
	}

	for (i = 0; i < TYPES; i++) {
		delete[] buffers[i];
	}
	delete[] buffers;

	return meshes;
}

