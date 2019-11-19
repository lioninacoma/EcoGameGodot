#include "chunk.h"

using namespace godot;

void Chunk::_register_methods() {
	register_method("getOffset", &Chunk::getOffset);
	register_method("getVolume", &Chunk::getVolume);
	register_method("getVoxel", &Chunk::getVoxel);
	register_method("getMeshInstanceId", &Chunk::getMeshInstanceId);
	register_method("getVoxelRay", &Chunk::getVoxelRay);
	register_method("setOffset", &Chunk::setOffset);
	register_method("setVoxel", &Chunk::setVoxel);
	register_method("setMeshInstanceId", &Chunk::setMeshInstanceId);
	register_method("buildVolume", &Chunk::buildVolume);
}

Chunk::Chunk(Vector3 offset) {
	Chunk::offset = offset;
	Chunk::noise = OpenSimplexNoise::_new();
}

Chunk::~Chunk() {

}

void Chunk::_init() {
	Chunk::volume = new char[BUFFER_SIZE];

	Chunk::noise->set_seed(NOISE_SEED);
	Chunk::noise->set_octaves(3);
	Chunk::noise->set_period(60.0);
	Chunk::noise->set_persistence(0.5);
}

int Chunk::getVoxel(int x, int y, int z) {
	if (x < 0 || x >= CHUNK_SIZE_X) return 0;
	if (y < 0 || y >= CHUNK_SIZE_Y) return 0;
	if (z < 0 || z >= CHUNK_SIZE_Z) return 0;
	return (int) volume[flattenIndex(x, y, z)];
}

int Chunk::getVoxelNoiseY(int x, int z) {
	float scale = 0.25;
	float y = noise->get_noise_2d(
		(x + offset.x) * scale, 
		(z + offset.z) * scale) / 2.0 + 0.5;
	y *= CHUNK_SIZE_Y;
	return (int) y;
}

int Chunk::flattenIndex(int x, int y, int z) {
	return x + CHUNK_SIZE_X * (y + CHUNK_SIZE_Y * z);
}

float Chunk::getVoxelNoiseChance(int x, int y, int z) {
	float scale = 1.0;
	return noise->get_noise_3d(
		(x + offset.x) * scale, 
		(y + offset.y) * scale, 
		(z + offset.z) * scale) / 2.0 + 0.5;
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
	volume[flattenIndex(x, y, z)] = (char)v;
}

int Chunk::buildVolume() {
	memset(volume, 0, BUFFER_SIZE);
	int amountVoxel = 0;

	for (int z = 0; z < CHUNK_SIZE_Z; z++) {
		for (int x = 0; x < CHUNK_SIZE_X; x++) {
			int y = getVoxelNoiseY(x, z);
			//Godot::print(String("y: {0}").format(Array::make(y)));
			for (int i = 0; i < y; i++) {
				float c = getVoxelNoiseChance(x, i, z);
				//Godot::print(String("c: {0}").format(Array::make(c)));
				float t = 0.7;
				if (c < t) {
					setVoxel(x, i, z, 1);
					amountVoxel++;
				}
			}
		}
	}

	return amountVoxel;
}