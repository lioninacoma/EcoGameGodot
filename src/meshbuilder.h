#ifndef MESHBUILDER_H
#define MESHBUILDER_H

#include <String.hpp>
#include <Vector3.hpp>

#include <iostream>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include "constants.h"
#include "objectpool.h"
#include "chunk.h"

namespace bpt = boost::posix_time;
using namespace std;

namespace godot {

	class MeshBuilder {
	private:
		const int SOUTH = 0;
		const int NORTH = 1;
		const int EAST = 2;
		const int WEST = 3;
		const int TOP = 4;
		const int BOTTOM = 5;
		const int DIMS[3] = { CHUNK_SIZE_X, CHUNK_SIZE_Y, CHUNK_SIZE_Z };

		static ObjectPool<int, BUFFER_SIZE, POOL_SIZE>& getMaskPool() {
			static ObjectPool<int, BUFFER_SIZE, POOL_SIZE> pool;
			return pool;
		};

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

		vector<int> buildVertices(Chunk* chunk, float** buffers, int buffersLen);
	};

}

#endif
