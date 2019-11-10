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
	Chunk::volume = new PoolByteArray();
	Chunk::volume->resize(BUFFER_SIZE);

	Chunk::noise->set_seed(NOISE_SEED);
	Chunk::noise->set_octaves(4);
	Chunk::noise->set_period(60.0);
	Chunk::noise->set_persistence(0.5);
}

Vector3 Chunk::getOffset() {
	return Chunk::offset;
}

PoolByteArray* Chunk::getVolume() {
	return Chunk::volume;
}

void Chunk::setOffset(Vector3 offset) {
	Chunk::offset = offset;
}

int Chunk::getVoxelNoiseY(Vector3 offset, int x, int z) {
	Vector2 noise2DV = Vector2(x + offset.x, z + offset.z) * 0.25;
	float y = noise->get_noise_2dv(noise2DV) / 2.0 + 0.5;
	y *= CHUNK_SIZE_Y;
	return (int)y;
}

float Chunk::getVoxelNoiseChance(Vector3 offset, int x, int y, int z) {
	Vector3 noise3DV = Vector3(x + offset.x, y + offset.y, z + offset.z) * 1.5;
	return noise->get_noise_3dv(noise3DV) / 2.0 + 0.5;
}

int Chunk::flattenIndex(int x, int y, int z) {
	return x + CHUNK_SIZE_X * (y + CHUNK_SIZE_Y * z);
}

PoolByteArray* Chunk::setVoxel(PoolByteArray* volume, int x, int y, int z, char v) {
	if (x < 0 || x >= CHUNK_SIZE_X) return volume;
	if (y < 0 || y >= CHUNK_SIZE_Y) return volume;
	if (z < 0 || z >= CHUNK_SIZE_Z) return volume;
	volume->set(flattenIndex(x, y, z), v);
	return volume;
}

void Chunk::buildVolume() {
	for (int i = 0; i < BUFFER_SIZE; i++) volume->set(i, 0);

	for (int z = 0; z < CHUNK_SIZE_Z; z++) {
		for (int x = 0; x < CHUNK_SIZE_X; x++) {
			int y = getVoxelNoiseY(offset, x, z);
			//Godot::print(String("y: {0}").format(Array::make(y)));
			for (int i = 0; i < y; i++) {
				float c = getVoxelNoiseChance(offset, x, i, z);
				//Godot::print(String("c: {0}").format(Array::make(c)));
				float t = 0.5;
				volume = setVoxel(volume, x, i, z, 1);
			}
		}
	}

	//for (int i = 0; i < BUFFER_SIZE; i++) volume->set(i, 1);
}