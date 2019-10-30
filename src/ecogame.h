#ifndef GDEXAMPLE_H
#define GDEXAMPLE_H

#include <Godot.hpp>
#include <Reference.hpp>
#include <String.hpp>
#include <Array.hpp>
#include <string.h>
#include <vector>
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
	PoolByteArray volume;
	int dims[3];
	int flattenIndex(int x, int y, int z);
	unsigned char getType(int x, int y, int z);
	float getU(int type);
	float getV(int type);
	void quad(PoolIntArray offset, int bl[], int tl[], int tr[], int br[], Array vertices, int side, int type);
	void createTop(PoolIntArray offset, int bl[], int tl[], int tr[], int br[], Array vertices, int type);
	void createBottom(PoolIntArray offset, int bl[], int tl[], int tr[], int br[], Array vertices, int type);
	void createLeft(PoolIntArray offset, int bl[], int tl[], int tr[], int br[], Array vertices, int type);
	void createRight(PoolIntArray offset, int bl[], int tl[], int tr[], int br[], Array vertices, int type);
	void createFront(PoolIntArray offset, int bl[], int tl[], int tr[], int br[], Array vertices, int type);
	void createBack(PoolIntArray offset, int bl[], int tl[], int tr[], int br[], Array vertices, int type);
public:
	static void _register_methods();

	EcoGame();
	~EcoGame();

	void _init(); // our initializer called by Godot

	Array buildVertices(PoolByteArray volume, PoolIntArray offset);
};

}

#endif
