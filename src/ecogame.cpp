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
	register_method("buildSections", &EcoGame::buildSections);
	register_method("navigate", &EcoGame::navigate);
	register_method("navigateToClosestVoxel", &EcoGame::navigateToClosestVoxel);
	register_method("findVoxelsInRange", &EcoGame::findVoxelsInRange);
	register_method("updateGraph", &EcoGame::updateGraph);
	register_method("updateGraphs", &EcoGame::updateGraphs);
}

EcoGame::EcoGame() {
	chunkBuilder = new ChunkBuilder();
	sections = new Section * [SECTIONS_LEN * sizeof(*sections)];

	memset(sections, 0, SECTIONS_LEN * sizeof(*sections));
}

EcoGame::~EcoGame() {
	delete[] sections;
	delete chunkBuilder;
}

void EcoGame::_init() {
	// initialize any variables here
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
	Navigator::get()->navigate(startV, goalV, actorInstanceId, game);
}

void EcoGame::navigateToClosestVoxelTask(Vector3 startV, int voxel, int actorInstanceId, Node* game, EcoGame* lib) {
	Navigator::get()->navigateToClosestVoxel(startV, voxel, actorInstanceId, game, this);
}

void EcoGame::setVoxel(Vector3 position, int voxel) {
	Node* game = get_tree()->get_root()->get_node("EcoGame");
	Vector3 p = position;
	Section* section;
	Chunk* chunk;
	p = fn::toSectionCoords(p);
	section = sections[fn::fi2((int)p.x, (int)p.z, SECTIONS_SIZE)];
	if (!section) return;
	section->setVoxel(chunkBuilder, game, position, voxel);
}

int EcoGame::getVoxel(Vector3 position) {
	Vector3 p = position;
	Section* section;
	Chunk* chunk;
	p = fn::toSectionCoords(p);
	section = sections[fn::fi2((int)p.x, (int)p.z, SECTIONS_SIZE)];
	if (!section) return 0;
	return section->getVoxel(position);
}

void EcoGame::updateGraph(Variant vChunk) {
	Node* game = get_tree()->get_root()->get_node("EcoGame");
	Chunk* chunk = as<Chunk>(vChunk.operator Object * ());

	ThreadPool::get()->submitTask(boost::bind(&EcoGame::updateGraphTask, this, chunk, game));
}

void EcoGame::updateGraphTask(Chunk* chunk, Node* game) {
	Navigator::get()->updateGraph(chunk, game);
}

void EcoGame::updateGraphs(Vector3 center, float radius) {
	Node* game = get_tree()->get_root()->get_node("EcoGame");
	int x, z, i;
	int dist = radius / (SECTION_SIZE * CHUNK_SIZE_X);
	Vector3 p = Vector3(center.x, 0, center.z);
	Section* section;
	
	p = fn::toSectionCoords(p);

	for (z = -dist + (int)p.z; z <= dist + (int)p.z; z++) {
		for (x = -dist + (int)p.x; x <= dist + (int)p.x; x++) {
			if (x < 0 || z < 0 || x >= SECTIONS_SIZE || z >= SECTIONS_SIZE) continue;
			if (p.distance_to(Vector3(x, 0, z)) > dist + 1) continue;
			if (!sections[fn::fi2(x, z, SECTIONS_SIZE)]) continue;

			section = sections[fn::fi2(x, z, SECTIONS_SIZE)];
			
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
	Section* section;
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
				section = new Section(Vector2(x * SECTION_SIZE, z * SECTION_SIZE));
				sections[fn::fi2(x, z, SECTIONS_SIZE)] = section;
				ThreadPool::get()->submitTask(boost::bind(&EcoGame::buildSection, this, section, game));

				if (++sectionsBuilt >= maxSectionsBuilt) return;
			}
		}
		dist++;
	}
}

void EcoGame::buildSection(Section* section, Node* game) {
	section->build(chunkBuilder, game);
}

void EcoGame::buildChunk(Variant vChunk) {
	Chunk* chunk = as<Chunk>(vChunk.operator Object * ());
	Node* game = get_tree()->get_root()->get_node("EcoGame");
	chunkBuilder->build(chunk, game);
}

PoolVector3Array EcoGame::findVoxelsInRange(Vector3 startV, float radius, int voxel) {
	Section* section;

	Vector3 start = startV - Vector3(radius, radius, radius);
	Vector3 end = startV + Vector3(radius, radius, radius);
	Vector3 tl = start;
	Vector3 br = end;

	tl = fn::toChunkCoords(tl);
	br = fn::toChunkCoords(br);

	Vector2 offset = Vector2(tl.x, tl.z);
	int sectionSize = max(abs(br.x - tl.x), abs(br.z - tl.z)) + 1;

	section = new Section(offset, sectionSize);
	section->fill(sections, SECTION_SIZE, SECTIONS_SIZE);
	PoolVector3Array voxels = section->findVoxelsInRange(startV, radius, voxel);
	delete section;
	return voxels;
}

bool EcoGame::voxelAssetFits(Vector3 startV, int type) {
	VoxelAssetType vat = (VoxelAssetType)type;
	VoxelAsset* asset = VoxelAssetManager::get()->getVoxelAsset(vat);
	float width = asset->getWidth();
	float depth = asset->getDepth();
	Section* section;
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

	section = new Section(offset, sectionSize);
	section->fill(sections, SECTION_SIZE, SECTIONS_SIZE);
	bool fits = section->voxelAssetFits(start, vat);
	delete section;
	return fits;
}

void EcoGame::addVoxelAsset(Vector3 startV, int type) {
	VoxelAssetType vat = (VoxelAssetType)type;
	VoxelAsset* asset = VoxelAssetManager::get()->getVoxelAsset(vat);
	Node* game = get_tree()->get_root()->get_node("EcoGame");
	float width = asset->getWidth();
	float depth = asset->getDepth();
	Section* section;
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

	section = new Section(offset, sectionSize);
	section->fill(sections, SECTION_SIZE, SECTIONS_SIZE);
	section->addVoxelAsset(start, vat, chunkBuilder, game);
	delete section;
}

Array EcoGame::buildVoxelAsset(int type) {
	MeshBuilder meshBuilder;
	VoxelAssetType vat = (VoxelAssetType)type;
	VoxelAsset* asset = VoxelAssetManager::get()->getVoxelAsset(vat);
	Array meshes;

	const int BUFFER = asset->getWidth() * asset->getHeight() * asset->getDepth();
	const int MAX_VERTICES = (BUFFER * VERTEX_SIZE * 6 * 4) / 2;
	float** buffers = new float*[TYPES];

	for (int i = 0; i < TYPES; i++) {
		buffers[i] = new float[MAX_VERTICES];
		memset(buffers[i], 0, MAX_VERTICES * sizeof(*buffers[i]));
	}

	vector<int> offsets = meshBuilder.buildVertices(vat, buffers, TYPES);

	for (int o = 0; o < offsets.size(); o++) {
		int offset = offsets[o];
		if (offset <= 0) continue;

		float* vertices = buffers[o];
		int amountVertices = offset / VERTEX_SIZE;
		int amountIndices = amountVertices / 2 * 3;

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

		for (int i = 0, n = 0; i < offset; i += VERTEX_SIZE, n++) {
			vertexArrayWrite[n] = Vector3(vertices[i], vertices[i + 1], vertices[i + 2]);
			normalArrayWrite[n] = Vector3(vertices[i + 3], vertices[i + 4], vertices[i + 5]);
			uvArrayWrite[n] = Vector2(vertices[i + 6], vertices[i + 7]);
		}

		for (int i = 0, j = 0; i < amountIndices; i += 6, j += 4) {
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

	for (int i = 0; i < TYPES; i++) {
		delete [] buffers[i];
	}
	delete[] buffers;

	return meshes;
}

