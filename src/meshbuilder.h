#ifndef MESHBUILDER_H
#define MESHBUILDER_H

#include <String.hpp>
#include <Vector3.hpp>

#include "constants.h"
#include "objectpool.h"
#include "chunk.h"

using namespace std;

namespace godot {

	class MeshBuilder {
	private:
		static ObjectPool<int, BUFFER_SIZE, POOL_SIZE>& getMaskPool() {
			static ObjectPool<int, BUFFER_SIZE, POOL_SIZE> pool;
			return pool;
		};
		int dims[3] = { CHUNK_SIZE_X, CHUNK_SIZE_Y, CHUNK_SIZE_Z };

		float getU(int type);
		float getV(int type);
		int quad(Vector3 offset, int bl[], int tl[], int tr[], int br[], float* vertices, int side, int type, int vertexOffset);
		int createTop(Vector3 offset, int bl[], int tl[], int tr[], int br[], float* vertices, int type, int vertexOffset);
		int createBottom(Vector3 offset, int bl[], int tl[], int tr[], int br[], float* vertices, int type, int vertexOffset);
		int createLeft(Vector3 offset, int bl[], int tl[], int tr[], int br[], float* vertices, int type, int vertexOffset);
		int createRight(Vector3 offset, int bl[], int tl[], int tr[], int br[], float* vertices, int type, int vertexOffset);
		int createFront(Vector3 offset, int bl[], int tl[], int tr[], int br[], float* vertices, int type, int vertexOffset);
		int createBack(Vector3 offset, int bl[], int tl[], int tr[], int br[], float* vertices, int type, int vertexOffset);
	public:
		MeshBuilder();
		~MeshBuilder();

		int buildVertices(Chunk* chunk, float* out);
	};

}

#endif
