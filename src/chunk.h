#ifndef CHUNK_H
#define CHUNK_H

#include <Godot.hpp>
#include <Spatial.hpp>
#include <OpenSimplexNoise.hpp>

#include "constants.h"

namespace godot {

	class Chunk : public Spatial {
		GODOT_CLASS(Chunk, Spatial)

	private:
		OpenSimplexNoise* noise;
		Vector3 offset;
		PoolByteArray *volume;

		int getVoxelNoiseY(Vector3 offset, int x, int z);
		float getVoxelNoiseChance(Vector3 offset, int x, int y, int z);
	public:
		static void _register_methods();

		Chunk() : Chunk(Vector3(0, 0, 0)) {};
		Chunk(Vector3 offset);
		~Chunk();

		void _init(); // our initializer called by Godot

		Vector3 getOffset();
		PoolByteArray *getVolume();
		void setOffset(Vector3 offset);
		void buildVolume();
	};

}

#endif
