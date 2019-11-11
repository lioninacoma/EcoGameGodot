#ifndef MESHBUILDER_H
#define MESHBUILDER_H

#include <String.hpp>
#include <Variant.hpp>
#include <Vector2.hpp>
#include <Vector3.hpp>
#include <Array.hpp>
#include <PoolArrays.hpp>

#include <list>
#include <queue>
#include <iostream>

//#include <boost/thread/mutex.hpp>
//#include <boost/pool/pool_alloc.hpp>

#include "constants.h"
#include "objectpool.h"

//typedef boost::fast_pool_allocator<int[BUFFER_SIZE]> BufferAllocator;
//typedef boost::singleton_pool<boost::fast_pool_allocator_tag, sizeof(int[BUFFER_SIZE])> BufferAllocatorPool;

using namespace std;

namespace godot {

	/*class BufferPool {
	private:
		boost::mutex mutex;
		queue<int*, list<int*, BufferAllocator>> pool;
	public:
		static BufferPool& get() { static BufferPool pool; return pool; }
		
		BufferPool() {
			for (int i = 0; i < POOL_SIZE; i++) {
				pool.push(new int[BUFFER_SIZE]);
			}
		};
		~BufferPool() {
			BufferAllocatorPool::purge_memory();
		};
		int* borrow() {
			mutex.lock();
			int* o = pool.front();
			pool.pop();
			mutex.unlock();
			return o;
		};
		void ret(int* o) {
			mutex.lock();
			pool.push(o);
			mutex.unlock();
		};
	};*/

	class MeshBuilder {
	private:
		static ObjectPool<int>& getMaskPool() {
			static ObjectPool<int> pool;
			return pool;
		};
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
	public:
		MeshBuilder();
		~MeshBuilder();

		int buildVertices(Vector3 offset, PoolByteArray* volume, float* out);
	};

}

#endif
