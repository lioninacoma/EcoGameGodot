#include "chunk.h"

using namespace godot;

void Chunk::_register_methods() {
	register_method("getOffset", &Chunk::getOffset);
	register_method("getVolume", &Chunk::getVolume);
	register_method("setOffset", &Chunk::setOffset);
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

Vector3 Chunk::getOffset() {
	return Chunk::offset;
}

char* Chunk::getVolume() {
	return Chunk::volume;
}

char Chunk::getVoxel(int x, int y, int z) {
	if (x < 0 || x >= CHUNK_SIZE_X) return 0;
	if (y < 0 || y >= CHUNK_SIZE_Y) return 0;
	if (z < 0 || z >= CHUNK_SIZE_Z) return 0;
	return volume[flattenIndex(x, y, z)];
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
	float scale = 1.5;
	return noise->get_noise_3d(
		(x + offset.x) * scale, 
		(y + offset.y) * scale, 
		(z + offset.z) * scale) / 2.0 + 0.5;
}

void Chunk::setOffset(Vector3 offset) {
	Chunk::offset = offset;
}

void Chunk::setVoxel(int x, int y, int z, char v) {
	if (x < 0 || x >= CHUNK_SIZE_X) return;
	if (y < 0 || y >= CHUNK_SIZE_Y) return;
	if (z < 0 || z >= CHUNK_SIZE_Z) return;
	volume[flattenIndex(x, y, z)] = v;
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
				float t = 0.6;
				//if (c < t) {
					setVoxel(x, i, z, 1);
					amountVoxel++;
				//}
			}
		}
	}

	return amountVoxel;
}