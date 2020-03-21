#include "chunk.h"
#include "navigator.h"
#include "section.h"
#include "ecogame.h"

using namespace godot;

#define CHUNK_PADDING 0

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
	Chunk::volume = std::shared_ptr<VoxelData>(new VoxelData(CHUNK_SIZE_X + CHUNK_PADDING, CHUNK_SIZE_Y + CHUNK_PADDING, CHUNK_SIZE_Z + CHUNK_PADDING));
	Chunk::nodes = new unordered_map<size_t, std::shared_ptr<GraphNavNode>>();
}

Chunk::~Chunk() {
	volume.reset();
	delete nodes;
}

void Chunk::_init() {
	Chunk::noise = OpenSimplexNoise::_new();
	Chunk::noise->set_seed(NOISE_SEED);
	Chunk::noise->set_octaves(4);
	Chunk::noise->set_period(128.0);
	Chunk::noise->set_persistence(0.5);
}

float Chunk::isVoxel(int ix, int iy, int iz) {
	float d = 0.5;
	float cx = ix + offset.x;
	float cy = iy + offset.y;
	float cz = iz + offset.z;
	float x = cx / (WORLD_SIZE * CHUNK_SIZE_X);
	float y = cy / (CHUNK_SIZE_Y);
	float z = cz / (WORLD_SIZE * CHUNK_SIZE_Z);
	float s = pow(x - d, 2) + pow(y - d, 2) + pow(z - d, 2) - 0.1;
	s += noise->get_noise_3d(cx, cy, cz) / 8;
	//return floorf(s * 600.0) / 600.0;
	return s;
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

	std::shared_ptr<GraphNavNode> currentNode;
	Vector3 position;
	Vector3 nodePosition;
	
	if (v > 0) {
		if (navigatable) {
			try {
				position = offset + Vector3(x, y, z);
				nodePosition = position + Vector3(0.5, 0.5, 0.5);
				currentNode = getNode(fn::hash(nodePosition));

				if (currentNode) {
					Godot::print(String("remove node at: {0}").format(Array::make(nodePosition)));
					Navigator::get()->removeNode(currentNode);
					removeNode(currentNode);
					updateNodesAt(position);
				}
			}
			catch (const std::exception & e) {
				std::cerr << boost::diagnostic_information(e);
			}
		}
	}
	else {
		if (navigatable) {
			try {
				position = offset + Vector3(x, y, z);
				nodePosition = position + Vector3(0.5, 0.5, 0.5);
				currentNode = getNode(fn::hash(nodePosition));

				if (!currentNode) {
					Godot::print(String("add node at: {0}").format(Array::make(nodePosition)));
					auto gn = GraphNavNode::_new();
					gn->setPoint(nodePosition);
					gn->setVoxel(v);
					currentNode = std::shared_ptr<GraphNavNode>(gn);
					Navigator::get()->addNode(currentNode, this);
					addNode(currentNode);
					updateNodesAt(position);
				}
			}
			catch (const std::exception & e) {
				std::cerr << boost::diagnostic_information(e);
			}
		}
	}
}

std::shared_ptr<GraphNavNode> Chunk::getNode(size_t hash) {
	boost::shared_lock<std::shared_mutex> lock(CHUNK_NODES_MUTEX);
	auto it = Chunk::nodes->find(hash);
	if (it == Chunk::nodes->end()) return NULL;
	return it->second;
}

std::shared_ptr<GraphNavNode> Chunk::findNode(Vector3 position) {
	boost::shared_lock<std::shared_mutex> lock(CHUNK_NODES_MUTEX);
	std::shared_ptr<GraphNavNode> closest;
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
	auto lib = EcoGame::get();

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
			node = lib->getNode(Vector3(nx, ny, nz));
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
	auto lib = EcoGame::get();

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
					reachable = lib->getNode(Vector3(nx, ny, nz));
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
	auto lib = EcoGame::get();

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
					v = lib->getVoxel(Vector3(nx, ny, nz));
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
	auto lib = EcoGame::get();

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
					v = lib->getVoxel(Vector3(nx, ny, nz));
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

void Chunk::buildVolume() {
	int x, y, z, i, diff;
	float d, rd;
	float max_d = cog.length();

	for (z = 0; z < CHUNK_SIZE_Z + CHUNK_PADDING; z++) {
		for (y = 0; y < CHUNK_SIZE_Y + CHUNK_PADDING; y++) {
			for (x = 0; x < CHUNK_SIZE_X + CHUNK_PADDING; x++) {
				setVoxel(x, y, z, isVoxel(x, y, z));
			}
		}
	}

	volumeBuilt = true;
}

void Chunk::updateNodesAt(Vector3 position) {
	auto lib = EcoGame::get();

	int vx, vy, vz, x, y, z, i;
	size_t nHash;
	Vector3 chunkOffset, nodePosition, voxelPosition;
	std::shared_ptr<GraphNavNode> node;
	Chunk* context;

	vx = (int)position.x;
	vy = (int)position.y;
	vz = (int)position.z;
	char nv, v = getVoxel(
		vx % CHUNK_SIZE_X,
		vy % CHUNK_SIZE_Y,
		vz % CHUNK_SIZE_Z);

	// von Neumann neighbourhood (3D)
	int neighbours[6][4] = {
		{ 0,  1,  0,  0 },
		{-1,  0,  0,  2 },
		{ 0,  0,  1,  4 },
		{ 0, -1,  0,  1 },
		{ 1,  0,  0,  3 },
		{ 0,  0, -1,  5 }
	};

	for (i = 0; i < 6; i++) {
		x = neighbours[i][0];
		y = neighbours[i][1];
		z = neighbours[i][2];

		vx = position.x + x;
		vy = position.y + y;
		vz = position.z + z;

		voxelPosition = Vector3(vx, vy, vz);
		nodePosition = voxelPosition + Vector3(0.5, 0.5, 0.5);
		nHash = fn::hash(nodePosition);

		chunkOffset = nodePosition;
		chunkOffset = fn::toChunkCoords(chunkOffset);
		chunkOffset *= Vector3(CHUNK_SIZE_X, 0, CHUNK_SIZE_Z);

		if (offset != chunkOffset) {
			auto neighbour = lib->getChunk(nodePosition);
			if (neighbour == NULL) continue;
			context = neighbour.get();
		}
		else {
			context = this;
		}

		node = context->getNode(nHash);
		nv = context->getVoxel(
			vx % CHUNK_SIZE_X,
			vy % CHUNK_SIZE_Y,
			vz % CHUNK_SIZE_Z);

		if (v <= 0 && nv <= 0 && node) {
			node->setDirection(static_cast<GraphNavNode::DIRECTION>(neighbours[(i + 3) % 6][3]), false);
			
			if (!node->getAmountDirections()) {
				Navigator::get()->removeNode(node);
				context->removeNode(node);
				Godot::print(String("remove neighbour node at: {0}").format(Array::make(node->getPointU())));
			}
		}
		else if (v > 0 && nv <= 0) {
			if (!node) {
				auto gn = GraphNavNode::_new();
				gn->setPoint(nodePosition);
				gn->setVoxel(nv);
				node = std::shared_ptr<GraphNavNode>(gn);
				Navigator::get()->addNode(node, context);
				context->addNode(node);
			}
			else {
				Navigator::get()->addNode(node, context);
				context->addNode(node, static_cast<GraphNavNode::DIRECTION>(neighbours[(i + 3) % 6][3]));
			}

			Godot::print(String("add neighbour node at: {0}, nv: {1}").format(Array::make(node->getPointU(), (int)nv)));
		}
	}
}

void Chunk::addNode(std::shared_ptr<GraphNavNode> node, Vector3 normal) {
	boost::unique_lock<std::shared_mutex> lock(CHUNK_NODES_MUTEX);
	try {
		std::shared_ptr<GraphNavNode> current;
		size_t hash = node->getHash();
		node->determineGravity(cog);
		auto it = nodes->find(hash);

		if (it == nodes->end()) {
			nodes->emplace(hash, node);
			current = node;
		}
		else {
			current = it->second;
		}

		unsigned char mask = GraphNavNode::getDirectionMask(normal);
		current->setDirections(mask);
	}
	catch (const std::exception & e) {
		std::cerr << boost::diagnostic_information(e);
	}
}

void Chunk::addNode(std::shared_ptr<GraphNavNode> node, GraphNavNode::DIRECTION direction) {
	boost::unique_lock<std::shared_mutex> lock(CHUNK_NODES_MUTEX);
	try {
		std::shared_ptr<GraphNavNode> current;
		size_t hash = node->getHash();
		node->determineGravity(cog);
		auto it = nodes->find(hash);

		if (it == nodes->end()) {
			nodes->emplace(hash, node);
			current = node;
		}
		else {
			current = it->second;
		}

		current->setDirection(direction, true);
	}
	catch (const std::exception & e) {
		std::cerr << boost::diagnostic_information(e);
	}
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
			current = node;
		}
		else {
			current = it->second;
		}

		return;

		auto lib = EcoGame::get();
		Vector3 directionVector, voxelPosition, chunkOffset;
		unsigned char directionMask = 63; // 63 -> all 6 directions (2^6-1)
		unsigned char mask = directionMask;
		int d = -1, vx, vy, vz;
		char v;
		Chunk* context;

		while (mask && ++d < 6) {
			if (mask & 1) {
				directionVector = GraphNavNode::getDirectionVector(static_cast<GraphNavNode::DIRECTION>(d));
				voxelPosition = (current->getPointU() - Vector3(0.5, 0.5, 0.5)) + directionVector;
				vx = (int)voxelPosition.x;
				vy = (int)voxelPosition.y;
				vz = (int)voxelPosition.z;

				chunkOffset = voxelPosition;
				chunkOffset = fn::toChunkCoords(chunkOffset);
				chunkOffset *= Vector3(CHUNK_SIZE_X, 0, CHUNK_SIZE_Z);

				if (offset != chunkOffset) {
					auto neighbour = lib->getChunk(voxelPosition);
					if (neighbour == NULL) {
						mask >>= 1;
						continue;
					}
					context = neighbour.get();
				}
				else {
					context = this;
				}

				v = context->getVoxel(
					vx % CHUNK_SIZE_X,
					vy % CHUNK_SIZE_Y,
					vz % CHUNK_SIZE_Z);

				if (v) directionMask &= ~(1UL << d);
			}

			mask >>= 1;
		}

		current->clearDirections();
		current->setDirections(directionMask);
	}
	catch (const std::exception & e) {
		std::cerr << boost::diagnostic_information(e);
	}
}

void Chunk::removeNode(std::shared_ptr<GraphNavNode> node) {
	boost::unique_lock<std::shared_mutex> lock(CHUNK_NODES_MUTEX);
	try {
		size_t hash = node->getHash();
		if (Chunk::nodes->find(hash) == Chunk::nodes->end()) return;
		Chunk::nodes->erase(hash);
	}
	catch (const std::exception & e) {
		std::cerr << boost::diagnostic_information(e);
	}
}