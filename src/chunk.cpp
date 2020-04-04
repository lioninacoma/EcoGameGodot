#include "chunk.h"
#include "graphedge.h"
#include "graphnode.h"
#include "navigator.h"
#include "voxelworld.h"
#include "ecogame.h"

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
	Chunk::volume = std::shared_ptr<VoxelData>(new VoxelData(CHUNK_SIZE_X, CHUNK_SIZE_Y, CHUNK_SIZE_Z));
	Chunk::nodes = new unordered_map<size_t, std::shared_ptr<GraphNavNode>>();

	int amountVertices, amountIndices, amountFaces;
	const int VERTEX_BUFFER_SIZE = CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z * 6 * 4;
}

Chunk::~Chunk() {
	volume.reset();
	//delete nodes;
}

void Chunk::_init() {
	Chunk::noise = OpenSimplexNoise::_new();
	Chunk::noise->set_seed(NOISE_SEED);
	Chunk::noise->set_octaves(6);
	Chunk::noise->set_period(192.0);
	Chunk::noise->set_persistence(0.5);
}

float Chunk::getVoxel(int x, int y, int z) {
	return volume->get(x, y, z);
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

void Chunk::setVoxel(int x, int y, int z, float v) {
	volume->set(x, y, z, v);
	if (!navigatable) return;

	std::shared_ptr<GraphNavNode> node;
	int i, index = fn::fi3(x, y, z);
	float* vertex;

	std::function<void(int* face)> lambda = [&](auto next) {
		if (index == next[3]) {
			for (i = 0; i < 3; i++) {
				vertex = getVertex(next[i]);
				node = getNode(fn::hash(Vector3(vertex[0], vertex[1], vertex[2])));
				if (node) {
					//Godot::print(String("remove node at: {0}").format(Array::make(nodePosition)));
					removeNode(node);
				}
			}
		}
	};
	forEachFace(lambda);
}

std::shared_ptr<GraphNavNode> Chunk::getNode(size_t hash) {
	boost::shared_lock<std::shared_mutex> lock(CHUNK_NODES_MUTEX);
	auto it = Chunk::nodes->find(hash);
	if (it == Chunk::nodes->end()) return NULL;
	return it->second;
}

std::shared_ptr<GraphNavNode> Chunk::findNode(Vector3 position) {
	std::shared_ptr<GraphNavNode> closest;
	closest = getNode(fn::hash(position));
	if (closest) return closest;

	boost::shared_lock<std::shared_mutex> lock(CHUNK_NODES_MUTEX);
	float minDist = numeric_limits<float>::max();
	float dist;
	for (auto node : *nodes) {
		dist = node.second->getPoint()->distance_to(position);
		if (dist < minDist) {
			minDist = dist;
			closest = node.second;
		}
	}
	return closest;
}

//float Chunk::isVoxel(int ix, int iy, int iz) {
//	int width = world->getWidth();
//	int depth = world->getDepth();
//	float d = 0.5;
//	float cx = ix + offset.x;
//	float cy = iy + offset.y;
//	float cz = iz + offset.z;
//	float x = cx / (width * CHUNK_SIZE_X);
//	float y = cy / CHUNK_SIZE_Y;
//	float z = cz / (depth * CHUNK_SIZE_Z);
//	float s = pow(x - d, 2) + pow(y - d, 2) + pow(z - d, 2) - 0.08;
//	s += noise->get_noise_3d(cx, cy, cz) / 8;
//	//return floorf(s * 400.0) / 400.0;
//	return s;
//}

float Chunk::isVoxel(int ix, int iy, int iz) {
	float cx = ix + offset.x;
	float cy = iy + offset.y;
	float cz = iz + offset.z;
	float y = cy / CHUNK_SIZE_Y;
	float s = y + noise->get_noise_3d(cx / 2, cy / 2, cz / 2);
	return s;
}

void Chunk::buildVolume() {
	int x, y, z, i, diff;
	float d, rd;
	float max_d = cog.length();

	for (z = 0; z < CHUNK_SIZE_Z; z++) {
		for (y = 0; y < CHUNK_SIZE_Y; y++) {
			for (x = 0; x < CHUNK_SIZE_X; x++) {
				setVoxel(x, y, z, isVoxel(x, y, z));
			}
		}
	}

	volumeBuilt = true;
}

void Chunk::addNode(std::shared_ptr<GraphNavNode> node) {
	boost::unique_lock<std::shared_mutex> lock(CHUNK_NODES_MUTEX);
	try {
		std::shared_ptr<GraphNavNode> current;
		size_t hash = node->getHash();
		node->determineGravity(cog);
		auto it = nodes->find(hash);

		if (it == nodes->end()) {
			nodes->emplace(hash, node);
		}
	}
	catch (const std::exception & e) {
		std::cerr << boost::diagnostic_information(e);
	}
}

void Chunk::removeNode(std::shared_ptr<GraphNavNode> node) {
	boost::unique_lock<std::shared_mutex> lock(CHUNK_NODES_MUTEX);
	try {
		size_t hash = node->getHash();
		auto it = nodes->find(hash);

		if (it != nodes->end()) {
			nodes->erase(hash);
		}
	}
	catch (const std::exception & e) {
		std::cerr << boost::diagnostic_information(e);
	}
}

void Chunk::addEdge(std::shared_ptr<GraphNavNode> a, std::shared_ptr<GraphNavNode> b, float cost) {
	// ~600 MB
	auto edge = std::shared_ptr<GraphEdge>(new GraphEdge(a, b, cost));
	a->addEdge(edge);
	b->addEdge(edge);
}

std::shared_ptr<GraphNavNode> Chunk::fetchOrCreateNode(Vector3 position) {
	std::shared_ptr<GraphNavNode> node;
	size_t hash = fn::hash(position);
	Vector3 chunkOffset = position;
	chunkOffset = fn::toChunkCoords(chunkOffset);
	chunkOffset *= Vector3(CHUNK_SIZE_X, 0, CHUNK_SIZE_Z);

	if (offset != chunkOffset) {
		auto neighbourChunk = world->getChunk(position);
		if (neighbourChunk != NULL) {
			node = neighbourChunk->getNode(hash);
		}
	}
	else {
		node = getNode(hash);
	}

	if (!node) {
		node = std::shared_ptr<GraphNavNode>(GraphNavNode::_new());
		node->setPoint(position);
		node->setVoxel(1);
		addNode(node);
	}

	return node;
}

const bool SHOW_NODES_DEBUG = false;
void Chunk::addFaceNodes(Vector3 a, Vector3 b, Vector3 c) {
	std::shared_ptr<GraphNavNode> aNode, bNode, cNode;
	aNode = fetchOrCreateNode(a);
	bNode = fetchOrCreateNode(b);
	cNode = fetchOrCreateNode(c);

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