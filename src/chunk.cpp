#include "chunk.h"
#include "navigator.h"
#include "section.h"
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
	Chunk::surfaceY = new int[CHUNK_SIZE_X * CHUNK_SIZE_Z];
	Chunk::nodes = new unordered_map<size_t, std::shared_ptr<GraphNavNode>>();

	memset(surfaceY, 0, CHUNK_SIZE_X * CHUNK_SIZE_Z * sizeof(*surfaceY));
}

Chunk::~Chunk() {
	volume.reset();
	delete surfaceY;
	delete nodes;
}

void Chunk::_init() {
	Chunk::noise = OpenSimplexNoise::_new();
	Chunk::noise->set_seed(NOISE_SEED);
	Chunk::noise->set_octaves(4);
	Chunk::noise->set_period(128.0);
	Chunk::noise->set_persistence(0.5);

	Chunk::paddingNoise = OpenSimplexNoise::_new();
	Chunk::paddingNoise->set_seed(NOISE_SEED);
	Chunk::paddingNoise->set_octaves(1);
	Chunk::paddingNoise->set_period(4.0);
}

const float NOISE_CHANCE = 1.0;
float Chunk::getVoxelChance(int x, int y, int z) {
	return noise->get_noise_3d(
		(x + offset.x) * NOISE_CHANCE,
		(y + offset.y) * NOISE_CHANCE,
		(z + offset.z) * NOISE_CHANCE) / 2.0 + 0.5;
}

float chance(double x, double p) {
	return sqrt(((1.0 / p) - (2.0 * x - 1.0) * (2.0 * x - 1.0)) * p);
}

float redistribute(double x) {
	return pow(2, x) / 16.0;
}

bool Chunk::isVoxel(int x, int y, int z) {
	float max_dist = sqrt(cog.x * cog.x + cog.y * cog.y);
	float c = 1.0 - (Vector3(offset.x + x, y, offset.z + z).distance_to(cog) / max_dist);
	c *= max(min(getVoxelChance(x, y, z) + 0.4f, 1.0f), 0.0f);
	return c > 0.3;
}

//bool Chunk::isVoxel(int x, int y, int z) {
//	float section_width = CHUNK_SIZE_X * SECTION_SIZE;
//	float section_height = CHUNK_SIZE_Y;
//	float section_depth = CHUNK_SIZE_Z * SECTION_SIZE;
//
//	float section_x = sectionOffset.x * CHUNK_SIZE_X;
//	float section_z = sectionOffset.y * CHUNK_SIZE_Z;
//
//	float ix = ((offset.x - section_x) + x) / section_width;
//	float iy = y / section_height;
//	float iz = ((offset.z - section_z) + z) / section_depth;
//
//	float cx = chance(ix, padding);
//	float cy = chance(iy, padding);
//	float cz = chance(iz, padding);
//
//	float c = cx * cy * cz;
//
//	c *= getVoxelChance(x, y, z);
//	return c > 0.4;
//}

int Chunk::getVoxel(int x, int y, int z) {
	if (x < 0 || x >= CHUNK_SIZE_X) return 0;
	if (y < 0 || y >= CHUNK_SIZE_Y) return 0;
	if (z < 0 || z >= CHUNK_SIZE_Z) return 0;
	return (int) volume->get(x, y, z);
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

void Chunk::setSection(std::shared_ptr<Section> section) {
	Chunk::section = section;
	sectionOffset = section->getOffset();
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

int Chunk::getCurrentSurfaceY(int x, int z) {
	return getCurrentSurfaceY(fn::fi2(x, z));
}

int Chunk::getCurrentSurfaceY(int i) {
	return surfaceY[i];
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

void Chunk::setVoxel(int x, int y, int z, int v) {
	if (x < 0 || x >= CHUNK_SIZE_X) return;
	if (y < 0 || y >= CHUNK_SIZE_Y) return;
	if (z < 0 || z >= CHUNK_SIZE_Z) return;
	
	volume->set(x, y, z, v);
	int dy = surfaceY[fn::fi2(x, z)];

	std::shared_ptr<GraphNavNode> currentNode;
	Vector3 position;
	Vector3 nodePosition;
	
	if (v == 0) {
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

		if (dy == y) {
			for (int i = y - 1; i >= 0; i--) {
				surfaceY[fn::fi2(x, z)]--;
				if (volume->get(x, i, z)) {
					break;
				}
			}
		}

		amountVoxel--;
		if (amountVoxel < 0) amountVoxel = 0;
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

		surfaceY[fn::fi2(x, z)] = max(dy, y);
		amountVoxel++;
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

int Chunk::buildVolume() {
	int x, y, z, i, diff;

	if (volumeBuilt) return amountVoxel;
	amountVoxel = 0;

	for (z = 0; z < CHUNK_SIZE_Z; z++) {
		for (x = 0; x < CHUNK_SIZE_X; x++) {
			for (y = 0; y < CHUNK_SIZE_Y; y++) {
				if (isVoxel(x, y, z)) {
					setVoxel(x, y, z, 2);
				}
			}
		}
	}

	volumeBuilt = true;
	return amountVoxel;
}

int Chunk::buildVolume2() {
	int x, y, z, i, diff;
	float c;

	if (volumeBuilt) return amountVoxel;
	amountVoxel = 0;

	for (z = 0; z < CHUNK_SIZE_Z; z++) {
		for (x = 0; x < CHUNK_SIZE_X; x++) {
			y = getVoxelY(x, z);
			
			// WATER LAYER
			if (y <= WATER_LEVEL) {
				for (i = y; i <= WATER_LEVEL; i++) {
					setVoxel(x, i, z, 6);
				}
			}

			for (i = 0; i < y; i++) {
				c = getVoxelChance(x, i, z);
				if (c < VOXEL_CHANCE_T) {
					diff = abs(y - i);

					// GRASS LAYER
					if (diff < 3) {
						setVoxel(x, i, z, 1);
					}
					// DIRT LAYER
					else if (diff < 12) {
						setVoxel(x, i, z, 3);
					}
					// STONE LAYER
					else {
						setVoxel(x, i, z, 2);
					}
				}
				// WATER LAYER
				else if (i <= WATER_LEVEL) {
					setVoxel(x, i, z, 6);
				}
			}
		}
	}

	volumeBuilt = true;
	return amountVoxel;
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

		if (v && nv && node) {
			node->setDirection(static_cast<GraphNavNode::DIRECTION>(neighbours[(i + 3) % 6][3]), false);
			
			if (!node->getAmountDirections()) {
				Navigator::get()->removeNode(node);
				context->removeNode(node);
				Godot::print(String("remove neighbour node at: {0}").format(Array::make(node->getPointU())));
			}
		}
		else if (!v && nv) {
			if (!node) {
				auto gn = GraphNavNode::_new();
				gn->setPoint(nodePosition);
				gn->setVoxel(nv);
				node = std::shared_ptr<GraphNavNode>(gn);
			}

			Navigator::get()->addNode(node, context);
			context->addNode(node, static_cast<GraphNavNode::DIRECTION>(neighbours[(i + 3) % 6][3]));
			int dir = neighbours[i][3];
			int ndir = neighbours[(i + 3) % 6][3];
			Godot::print(String("add neighbour node at: {0}, dir: {1}, neighbour dir: {2}").format(Array::make(node->getPointU(), dir, ndir)));
		}
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