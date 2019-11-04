#ifndef ECOGAME_H
#define ECOGAME_H

#include <Godot.hpp>
#include <Reference.hpp>
#include <OpenSimplexNoise.hpp>
#include <String.hpp>
#include <Array.hpp>

#include <string.h>
#include <vector>
#include "test.h"

using namespace std; 

#define CHUNK_SIZE_X 16
#define CHUNK_SIZE_Y 256
#define CHUNK_SIZE_Z 16
#define BUFFER_SIZE CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z

#define SOUTH  0
#define NORTH  1
#define EAST   2
#define WEST   3
#define TOP    4
#define BOTTOM 5

#define TEXTURE_SCALE 1.0 / 1.0
#define TEXTURE_ATLAS_LEN 3
#define TEXTURE_MODE 1

namespace godot {

	class EcoGame : public Reference {
		GODOT_CLASS(EcoGame, Reference)

	private:
		OpenSimplexNoise *noise;
		int dims[3] = {CHUNK_SIZE_X, CHUNK_SIZE_Y, CHUNK_SIZE_Z};

		int flattenIndex(int x, int y, int z);
		Vector3 position(int i);
		unsigned char getType(PoolByteArray volume, int x, int y, int z);
		float getU(int type);
		float getV(int type);
		void quad(Vector3 offset, int bl[], int tl[], int tr[], int br[], Array vertices, int side, int type);
		void createTop(Vector3 offset, int bl[], int tl[], int tr[], int br[], Array vertices, int type);
		void createBottom(Vector3 offset, int bl[], int tl[], int tr[], int br[], Array vertices, int type);
		void createLeft(Vector3 offset, int bl[], int tl[], int tr[], int br[], Array vertices, int type);
		void createRight(Vector3 offset, int bl[], int tl[], int tr[], int br[], Array vertices, int type);
		void createFront(Vector3 offset, int bl[], int tl[], int tr[], int br[], Array vertices, int type);
		void createBack(Vector3 offset, int bl[], int tl[], int tr[], int br[], Array vertices, int type);
		int getVoxelNoiseY(Vector3 offset, int x, int z);
		float getVoxelNoiseChance(Vector3 offset, int x, int y, int z);
		PoolByteArray setVoxel(PoolByteArray volume, int x, int y, int z, char v);
	public:
		static void _register_methods();

		EcoGame();
		~EcoGame();

		void _init(); // our initializer called by Godot

		Array buildVertices(Vector3 offset, PoolByteArray volume);
		PoolByteArray buildVolume(Vector3 offset, int seed);
		void test(Variant v);
	};

}

#endif
