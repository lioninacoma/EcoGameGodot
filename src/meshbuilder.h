#ifndef MESHBUILDER_H
#define MESHBUILDER_H

#include <String.hpp>
#include <Variant.hpp>
#include <Vector2.hpp>
#include <Vector3.hpp>
#include <Array.hpp>
#include <PoolArrays.hpp>

#include "constants.h"

namespace godot {

	class MeshBuilder {
	private:
		int dims[3] = { CHUNK_SIZE_X, CHUNK_SIZE_Y, CHUNK_SIZE_Z };

		int flattenIndex(int x, int y, int z);
		Vector3 position(int i);
		unsigned char getType(PoolByteArray* volume, int x, int y, int z);
		float getU(int type);
		float getV(int type);
		int quad(Vector3 offset, int bl[], int tl[], int tr[], int br[], float* vertices, int side, int type, int vertexOffset);
		int createTop(Vector3 offset, int bl[], int tl[], int tr[], int br[], float* vertices, int type, int vertexOffset);
		int createBottom(Vector3 offset, int bl[], int tl[], int tr[], int br[], float* vertices, int type, int vertexOffset);
		int createLeft(Vector3 offset, int bl[], int tl[], int tr[], int br[], float* vertices, int type, int vertexOffset);
		int createRight(Vector3 offset, int bl[], int tl[], int tr[], int br[], float* vertices, int type, int vertexOffset);
		int createFront(Vector3 offset, int bl[], int tl[], int tr[], int br[], float* vertices, int type, int vertexOffset);
		int createBack(Vector3 offset, int bl[], int tl[], int tr[], int br[], float* vertices, int type, int vertexOffset);
		PoolByteArray* setVoxel(PoolByteArray* volume, int x, int y, int z, char v);
	public:
		MeshBuilder();
		~MeshBuilder();

		int buildVertices(Vector3 offset, PoolByteArray* volume, float* out);
	};

}

#endif
