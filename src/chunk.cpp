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
	register_method("getMeshInstanceId", &Chunk::getMeshInstanceId);
	register_method("setOffset", &Chunk::setOffset);
	register_method("setMeshInstanceId", &Chunk::setMeshInstanceId);
}

Chunk::Chunk(Vector3 offset) {
	Chunk::offset = offset;
	Chunk::volume = std::make_unique<VoxelData>(CHUNK_VOLUME_SIZE, CHUNK_VOLUME_SIZE, CHUNK_VOLUME_SIZE);
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

VoxelPlane Chunk::getVoxelPlane(Vector3 v) {
	return getVoxelPlane(v.x, v.y, v.z);
}

VoxelPlane Chunk::getVoxelPlane(int x, int y, int z) {
	return volume->get(x, y, z);
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

void Chunk::setVoxelPlane(int x, int y, int z, VoxelPlane voxelData) {
	volume->set(x, y, z, voxelData);
}

float Chunk::getDensity(Vector3 v) {
	VoxelPlane plane = getVoxelPlane(v);
	return plane.dist;
}

Vector3 Chunk::getNormal(Vector3 v) {
	VoxelPlane plane = getVoxelPlane(v);
	return plane.normal;
}

float Chunk::sampleDensity(Vector3 v) {
	float sizeh = WORLD_SIZE * CHUNK_SIZE / 2.f; 
	float n = fastNoise.GetNoise(v.x, v.y, v.z) * 0.4 + 0.6;
	//float n = noise->get_noise_3dv(v) * 0.4 + 0.6;
	float sphere = Sphere(v, Vector3(sizeh, sizeh, sizeh), n * sizeh);
	return sphere;
}

Vector3 Chunk::sampleNormal(Vector3 v) {
	float H = 0.001f;
	const float dx = sampleDensity(v + Vector3(H, 0.f, 0.f)) - sampleDensity(v - Vector3(H, 0.f, 0.f));
	const float dy = sampleDensity(v + Vector3(0.f, H, 0.f)) - sampleDensity(v - Vector3(0.f, H, 0.f));
	const float dz = sampleDensity(v + Vector3(0.f, 0.f, H)) - sampleDensity(v - Vector3(0.f, 0.f, H));
	return Vector3(dx, dy, dz).normalized();
}

VoxelPlane Chunk::sampleVoxelPlane(Vector3 v) {
	return VoxelPlane(sampleDensity(v), sampleNormal(v));
}

VoxelPlane Chunk::sampleVoxelPlane(int x, int y, int z) {
	return sampleVoxelPlane(Vector3(x, y, z));
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

	for (z = 0; z < CHUNK_VOLUME_SIZE; z++)
		for (y = 0; y < CHUNK_VOLUME_SIZE; y++)
			for (x = 0; x < CHUNK_VOLUME_SIZE; x++) {
				setVoxelPlane(x, y, z, sampleVoxelPlane(offset + Vector3(x, y, z)));
			}

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