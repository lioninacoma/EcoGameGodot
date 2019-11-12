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
		char* volume;

		int getVoxelNoiseY(int x, int z);
		float getVoxelNoiseChance(int x, int y, int z);
		int flattenIndex(int x, int y, int z);
	public:
		static void _register_methods();

		Chunk() : Chunk(Vector3(0, 0, 0)) {};
		Chunk(Vector3 offset);
		~Chunk();

		void _init(); // our initializer called by Godot
		
		// getter
		Vector3 getOffset();
		char* getVolume();
		char getVoxel(int x, int y, int z);

		// setter
		void setOffset(Vector3 offset);
		void setVoxel(int x, int y, int z, char v);
		void buildVolume();
	};

}

#endif
