#include "chunk.h"

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
}

Chunk::~Chunk() {
	volume.reset();
	delete surfaceY;
	delete nodes;
	delete nodeChanges;
}

void Chunk::_init() {
	Chunk::volume = boost::shared_ptr<VoxelData>(new VoxelData(CHUNK_SIZE_X, CHUNK_SIZE_Y, CHUNK_SIZE_Z));
	Chunk::surfaceY = new int[CHUNK_SIZE_X * CHUNK_SIZE_Z];
	Chunk::nodes = new unordered_map<size_t, boost::shared_ptr<GraphNode>>();
	Chunk::nodeChanges = new unordered_map<size_t, bool>();

	memset(surfaceY, 0, CHUNK_SIZE_X * CHUNK_SIZE_Z * sizeof(*surfaceY));

	Chunk::noise = OpenSimplexNoise::_new();
	Chunk::noise->set_seed(NOISE_SEED);
	Chunk::noise->set_octaves(4);
	Chunk::noise->set_period(320.0);
	Chunk::noise->set_persistence(0.5);
}

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

float Chunk::getVoxelChance(int x, int y, int z) {
	return noise->get_noise_3d(
		(x + offset.x) * VOXEL_CHANCE_NOISE_SCALE,
		(y + offset.y) * VOXEL_CHANCE_NOISE_SCALE,
		(z + offset.z) * VOXEL_CHANCE_NOISE_SCALE) / 2.0 + 0.5;
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

int Chunk::getCurrentSurfaceY(int x, int z) {
	return getCurrentSurfaceY(fn::fi2(x, z));
}

int Chunk::getCurrentSurfaceY(int i) {
	return surfaceY[i];
}

Ref<Voxel> Chunk::getVoxelRay(Vector3 from, Vector3 to) {
	vector<Voxel*> list;
	Ref<Voxel> voxel = NULL;

	boost::function<Voxel*(int, int, int)> intersection(boost::bind(&Chunk::intersection, this, _1, _2, _3));
	list = Intersection::get<Voxel*>(from, to, true, intersection, list);
	
	if (!list.empty()) {
		voxel = Ref<Voxel>(list.front());
	}

	return voxel;
}

void Chunk::setVoxel(int x, int y, int z, int v) {
	if (x < 0 || x >= CHUNK_SIZE_X) return;
	if (y < 0 || y >= CHUNK_SIZE_Y) return;
	if (z < 0 || z >= CHUNK_SIZE_Z) return;
	
	volume->set(x, y, z, v);
	int dy = surfaceY[fn::fi2(x, z)];
	
	if (v == 0) {
		if (navigatable) {
			Vector3 node = offset + Vector3(x + 0.5, y + 1, z + 0.5);
			size_t hash = fn::hash(node);

			if (nodes->find(hash) != nodes->end()) {
				removeNode(node);
			}

			int voxelBelow = volume->get(x, y - 1, z);
			if (y - 1 >= 0 && voxelBelow && voxelBelow != 6) {
				Vector3 newNode = node + Vector3(0, -1, 0);
				addNode(boost::shared_ptr<GraphNode>(new GraphNode(newNode, v)));
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
			Vector3 node = offset + Vector3(x + 0.5, y, z + 0.5);
			size_t hash = fn::hash(node);

			if (nodes->find(hash) != nodes->end()) {
				removeNode(node);
			}

			if (y + 1 < CHUNK_SIZE_Y && !volume->get(x, y + 1, z) && v != 6) {
				Vector3 newNode = node + Vector3(0, 1, 0);
				addNode(boost::shared_ptr<GraphNode>(new GraphNode(newNode, v)));
			}
		}

		surfaceY[fn::fi2(x, z)] = max(dy, y);
		amountVoxel++;
	}
}

int Chunk::buildVolume() {
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