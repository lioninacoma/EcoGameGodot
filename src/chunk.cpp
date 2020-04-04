#include "chunk.h"
#include "navigator.h"
#include "section.h"
#include "voxelworld.h"

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

void Chunk::setSection(std::shared_ptr<Section> section) {
	Chunk::section = section;
};

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

vector<std::shared_ptr<GraphNavNode>> Chunk::getVoxelNodes(Vector3 voxelPosition) {
	vector<std::shared_ptr<GraphNavNode>> nodes;
	std::shared_ptr<GraphNavNode> node;
	Vector3 chunkOffset;
	int i, x, y, z;
	float nx, ny, nz;
	size_t nHash;

	// von Neumann neighbourhood (3D)
	int neighbours[7][3] = {
		{ 0,  0,  0 },
		{ 0,  1,  0 },
		{ 0, -1,  0 },
		{-1,  0,  0 },
		{ 1,  0,  0 },
		{ 0,  0,  1 },
		{ 0,  0, -1 }
	};

	// center voxel position
	voxelPosition += Vector3(0.5, 0.5, 0.5);

	for (i = 0; i < 7; i++) {
		x = neighbours[i][0];
		y = neighbours[i][1];
		z = neighbours[i][2];

		nx = voxelPosition.x + x;
		ny = voxelPosition.y + y;
		nz = voxelPosition.z + z;
		nHash = fn::hash(Vector3(nx, ny, nz));

		chunkOffset.x = nx;
		chunkOffset.y = ny;
		chunkOffset.z = nz;
		chunkOffset = fn::toChunkCoords(chunkOffset);
		chunkOffset *= Vector3(CHUNK_SIZE_X, 0, CHUNK_SIZE_Z);

		if (offset != chunkOffset) {
			node = world->getNode(Vector3(nx, ny, nz));
		}
		else {
			node = getNode(nHash);
		}

		if (!node) {
			continue;
		}

		nodes.push_back(node);
	}
	return nodes;
}

vector<std::shared_ptr<GraphNavNode>> Chunk::getReachableNodes(std::shared_ptr<GraphNavNode> node) {
	vector<std::shared_ptr<GraphNavNode>> nodes;
	std::shared_ptr<GraphNavNode> reachable;
	Vector3 position = fn::unreference(node->getPoint());
	Vector3 chunkOffset;
	int x, y, z, nx, ny, nz, v;
	size_t nHash;
	
	for (z = -1; z < 2; z++)
		for (y = -1; y < 2; y++)
			for (x = -1; x < 2; x++) {
				if (!x && !y && !z) continue;

				nx = position.x + x;
				ny = position.y + y;
				nz = position.z + z;
				nHash = fn::hash(Vector3(nx, ny, nz));

				chunkOffset.x = nx;
				chunkOffset.y = ny;
				chunkOffset.z = nz;
				chunkOffset = fn::toChunkCoords(chunkOffset);
				chunkOffset *= Vector3(CHUNK_SIZE_X, 0, CHUNK_SIZE_Z);

				if (offset != chunkOffset) {
					reachable = world->getNode(Vector3(nx, ny, nz));
				}
				else {
					reachable = getNode(nHash);
				}

				if (!reachable) {
					continue;
				}

				nodes.push_back(reachable);
			}
	return nodes;
}

PoolVector3Array Chunk::getReachableVoxels(Vector3 voxelPosition) {
	PoolVector3Array voxels;
	Vector3 chunkOffset;
	int x, y, z, nx, ny, nz, v;
	
	for (z = -1; z < 2; z++)
		for (y = -1; y < 2; y++)
			for (x = -1; x < 2; x++) {
				if (!x && !y && !z) continue;

				nx = voxelPosition.x + x;
				ny = voxelPosition.y + y;
				nz = voxelPosition.z + z;

				chunkOffset.x = nx;
				chunkOffset.y = ny;
				chunkOffset.z = nz;
				chunkOffset = fn::toChunkCoords(chunkOffset);
				chunkOffset *= Vector3(CHUNK_SIZE_X, 0, CHUNK_SIZE_Z);

				if (offset != chunkOffset) {
					v = world->getVoxel(Vector3(nx, ny, nz));
				}
				else {
					v = getVoxel(
						nx % CHUNK_SIZE_X,
						ny % CHUNK_SIZE_Y,
						nz % CHUNK_SIZE_Z);
				}

				if (!v) continue;
				voxels.push_back(Vector3(nx, ny, nz));
			}
	return voxels;
}

PoolVector3Array Chunk::getReachableVoxelsOfType(Vector3 voxelPosition, int type) {
	PoolVector3Array voxels;
	Vector3 chunkOffset;
	int x, y, z, nx, ny, nz, v;

	for (y = 2; y > -3; y--)
		for (z = -2; z < 3; z++)
			for (x = -2; x < 3; x++) {
				nx = voxelPosition.x + x;
				ny = voxelPosition.y + y;
				nz = voxelPosition.z + z;

				chunkOffset.x = nx;
				chunkOffset.y = ny;
				chunkOffset.z = nz;
				chunkOffset = fn::toChunkCoords(chunkOffset);
				chunkOffset *= Vector3(CHUNK_SIZE_X, 0, CHUNK_SIZE_Z);

				if (offset != chunkOffset) {
					v = world->getVoxel(Vector3(nx, ny, nz));
				}
				else {
					v = getVoxel(
						nx % CHUNK_SIZE_X,
						ny % CHUNK_SIZE_Y,
						nz % CHUNK_SIZE_Z);
				}

				if (v != type) continue;

				voxels.push_back(Vector3(nx, ny, nz));
			}
	return voxels;
}

//float Chunk::isVoxel(int ix, int iy, int iz) {
//	float d = 0.5;
//	float cx = ix + offset.x;
//	float cy = iy + offset.y;
//	float cz = iz + offset.z;
//	float x = cx / (WORLD_SIZE * CHUNK_SIZE_X);
//	float y = cy / CHUNK_SIZE_Y;
//	float z = cz / (WORLD_SIZE * CHUNK_SIZE_Z);
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

		// chunk and nav nodes need to be locked
		world->getNavigator()->addNode(node);
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

		// chunk and nav nodes need to be locked
		world->getNavigator()->removeNode(node);
	}
	catch (const std::exception & e) {
		std::cerr << boost::diagnostic_information(e);
	}
}