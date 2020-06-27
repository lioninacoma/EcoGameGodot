#include "chunk.h"
#include "graphedge.h"
#include "graphnode.h"
#include "navigator.h"
#include "voxelworld.h"
#include "ecogame.h"
#include "octree.h"

using namespace godot;

void Chunk::_register_methods() {
	register_method("getOffset", &Chunk::getOffset);
	register_method("getVoxel", &Chunk::getVoxel);
	register_method("getMeshInstanceId", &Chunk::getMeshInstanceId);
	register_method("setOffset", &Chunk::setOffset);
	register_method("setVoxel", &Chunk::setVoxel);
	register_method("setMeshInstanceId", &Chunk::setMeshInstanceId);
}

Chunk::Chunk(Vector3 offset) {
	Chunk::offset = offset;
	Chunk::volume = std::make_unique<VoxelData>(CHUNK_SIZE, CHUNK_SIZE, CHUNK_SIZE);
}

Chunk::~Chunk() {
	volume.reset();
	//delete nodes;
}

void Chunk::_init() {
	noise = OpenSimplexNoise::_new();
	noise->set_seed(NOISE_SEED);
	noise->set_octaves(6);
	noise->set_period(192.0);
	noise->set_persistence(0.5);

	fastNoise.SetNoiseType(FastNoise::PerlinFractal);
}

std::shared_ptr<VoxelWorld> Chunk::getWorld() {
	return world;
}

Vector3 Chunk::getOffset() {
	return offset;
}

int Chunk::getMeshInstanceId() {
	return meshInstanceId;
}

bool Chunk::isNavigatable() {
	return navigatable;
}

bool Chunk::isBuilding() {
	return building;
}

int Chunk::getAmountNodes() {
	boost::shared_lock<std::shared_mutex> lock(CHUNK_NODES_MUTEX);
	return nodes.size();
}

float Chunk::getVoxel(int x, int y, int z) {
	return volume->get(x, y, z);
}

Voxel* Chunk::intersection(int x, int y, int z) {
	int chunkX = (int) offset.x;
	int chunkY = (int) offset.y;
	int chunkZ = (int) offset.z;
	if (x >= chunkX && x < chunkX + CHUNK_SIZE
		&& y >= chunkY && y < chunkY + CHUNK_SIZE
		&& z >= chunkZ && z < chunkZ + CHUNK_SIZE) {
		int v = getVoxel(
			(int) x % CHUNK_SIZE,
			(int) y % CHUNK_SIZE,
			(int) z % CHUNK_SIZE);

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

Voxel* Chunk::getVoxelRay(Vector3 from, Vector3 to) {
	vector<Voxel*> list;
	Voxel* voxel = NULL;

	boost::function<Voxel*(int, int, int)> intersection(boost::bind(&Chunk::intersection, this, _1, _2, _3));
	list = Intersection::get<Voxel*>(from, to, true, intersection, list);
	
	if (!list.empty()) {
		voxel = list.front();
	}

	return voxel;
}

void Chunk::forEachNode(std::function<void(std::pair<size_t, std::shared_ptr<GraphNode>>)> func) {
	boost::shared_lock<std::shared_mutex> lock(CHUNK_NODES_MUTEX);
	std::for_each(nodes.begin(), nodes.end(), func);
}

void Chunk::setWorld(std::shared_ptr<VoxelWorld> world) {
	Chunk::world = world;
}

void Chunk::setOffset(Vector3 offset) {
	Chunk::offset = offset;
}

void Chunk::setNavigatable(bool navigatable) {
	Chunk::navigatable = navigatable;
}

void Chunk::setBuilding(bool building) {
	Chunk::building = building;
}

void Chunk::setMeshInstanceId(int meshInstanceId) {
	Chunk::meshInstanceId = meshInstanceId;
}

void Chunk::setIsVoxelFn(Ref<FuncRef> fnRef) {
	Chunk::isVoxelFn = fnRef;
}

void Chunk::setVoxel(int x, int y, int z, float v) {
	volume->set(x, y, z, v);
}

float Chunk::isVoxel(Vector3 v) {
	float sizeh = world->getSize() * CHUNK_SIZE / 2.f;
	float noise = fastNoise.GetNoise(v.x, v.y, v.z) * 0.4 + 0.6;
	float cube = Cuboid(v, Vector3(sizeh, sizeh, sizeh), Vector3(sizeh * 0.75, sizeh * 0.1, sizeh * 0.75));
	float sphere = Sphere(v, Vector3(sizeh, sizeh, sizeh), noise * sizeh);
	//return min(sphere, cube);
	return sphere;
	//float noise2D = fastNoise.GetNoise(v.x, v.z) * 0.5 + 0.5;
	//return v.y - 8.f * noise2D * 4;
}

float Chunk::isVoxel(int x, int y, int z) {
	return isVoxel(Vector3(x, y, z));
}

std::shared_ptr<GraphNode> Chunk::fetchNode(Vector3 position) {
	std::shared_ptr<GraphNode> node;
	node = getNode(fn::hash(position));

	if (!node) {
		node = world->getNode(position);
	}

	return node;
}

std::shared_ptr<GraphNode> Chunk::getNode(size_t hash) {
	boost::shared_lock<std::shared_mutex> lock(CHUNK_NODES_MUTEX);
	auto it = Chunk::nodes.find(hash);
	if (it == Chunk::nodes.end()) return NULL;
	return it->second;
}

void Chunk::buildVolume() {
	int x, y, z;

	for (z = 0; z < CHUNK_SIZE; z++)
		for (y = 0; y < CHUNK_SIZE; y++)
			for (x = 0; x < CHUNK_SIZE; x++)
				setVoxel(x, y, z, isVoxel(offset + Vector3(x, y, z)));

	volumeBuilt = true;
}

void Chunk::addNode(std::shared_ptr<GraphNode> node) {
	boost::unique_lock<std::shared_mutex> lock(CHUNK_NODES_MUTEX);
	size_t hash = node->getHash();

	auto chunk = world->getChunk(node->getPointU());
	if (chunk->getOffset() != offset) {
		chunk->addNode(node);
	}
	else if (nodes.find(hash) == nodes.end()) {
		nodes.emplace(hash, node);
	}
}

void Chunk::removeNode(std::shared_ptr<GraphNode> node) {
	boost::unique_lock<std::shared_mutex> lock(CHUNK_NODES_MUTEX);
	size_t hash = node->getHash();

	auto chunk = world->getChunk(node->getPointU());
	if (chunk->getOffset() != offset) {
		chunk->removeNode(node);
	}
	else if (nodes.find(hash) != nodes.end()) {
		node->clearEdges();
		nodes.erase(hash);
		node.reset();

		if (node.use_count() > 0 || node)
			cout << "node not deleted" << endl;
	}
}

void Chunk::addEdge(std::shared_ptr<GraphNode> a, std::shared_ptr<GraphNode> b, float cost) {
	if (*a == *b) return;

	auto abEdge = a->getEdgeWithNode(b->getHash());
	auto baEdge = b->getEdgeWithNode(a->getHash());

	if (abEdge && !baEdge) {
		b->addEdge(abEdge);
	}
	else if (!abEdge && baEdge) {
		a->addEdge(baEdge);
	}
	else if (!abEdge && !baEdge) {
		auto edge = std::make_shared<GraphEdge>(a, b, cost);
		a->addEdge(edge);
		b->addEdge(edge);
	}
}

std::shared_ptr<GraphNode> Chunk::fetchOrCreateNode(Vector3 position, Vector3 normal) {
	std::shared_ptr<GraphNode> node;

	node = fetchNode(position);

	if (!node) {
		node = std::make_shared<GraphNode>(position, normal);
		addNode(node);
	}

	return node;
}

const bool SHOW_NODES_DEBUG = false;
void Chunk::addFaceNodes(Vector3 a, Vector3 b, Vector3 c, Vector3 normal) {
	std::shared_ptr<GraphNode> aNode, bNode, cNode;
	aNode = fetchOrCreateNode(a, normal);
	bNode = fetchOrCreateNode(b, normal);
	cNode = fetchOrCreateNode(c, normal);

	addEdge(aNode, bNode, fn::euclidean(aNode->getPointU(), bNode->getPointU()));
	addEdge(aNode, cNode, fn::euclidean(aNode->getPointU(), cNode->getPointU()));
	addEdge(bNode, cNode, fn::euclidean(bNode->getPointU(), cNode->getPointU()));

	if (SHOW_NODES_DEBUG) {
		auto lib = EcoGame::get();
		Node* game = lib->getNode();
		ImmediateGeometry* geo;

		geo = ImmediateGeometry::_new();
		geo->begin(Mesh::PRIMITIVE_POINTS);
		geo->set_color(Color(1, 0, 0, 1));
		geo->add_vertex(aNode->getPointU());
		geo->add_vertex(bNode->getPointU());
		geo->add_vertex(cNode->getPointU());
		geo->end();
		game->call_deferred("draw_debug_dots", geo);

		geo = ImmediateGeometry::_new();
		geo->begin(Mesh::PRIMITIVE_LINES);
		geo->set_color(Color(0, 1, 0, 1));
		geo->add_vertex(aNode->getPointU());
		geo->add_vertex(bNode->getPointU());
		geo->add_vertex(bNode->getPointU());
		geo->add_vertex(cNode->getPointU());
		geo->add_vertex(cNode->getPointU());
		geo->add_vertex(aNode->getPointU());
		geo->end();
		game->call_deferred("draw_debug", geo);
	}
}

void Chunk::setRoot(std::shared_ptr<OctreeNode> root) {
	Chunk::root = root;
}

void Chunk::deleteRoot() {
	if (!root) return;
	DestroyOctree(root);
	root = NULL;
}

std::shared_ptr<OctreeNode> Chunk::getRoot() {
	return root;
}

vector<std::shared_ptr<OctreeNode>> Chunk::findNodes(FilterNodesFunc filterFunc) {
	vector<std::shared_ptr<OctreeNode>> nodes;
	Octree_FindNodes(root, filterFunc, nodes);
	return nodes;
}