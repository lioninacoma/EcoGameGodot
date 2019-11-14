#ifndef OBJECTPOOL_H
#define OBJECTPOOL_H

//#define SEGREGATED_STORAGE

#include <boost/pool/pool_alloc.hpp>
#include <boost/pool/simple_segregated_storage.hpp>
#include <boost/thread/mutex.hpp>

#include <vector>
#include <list>
#include <cstddef>
#include <cassert>
#include <iostream>

#include "semaphore.h"
#include "constants.h"

using namespace std;

#if defined(SEGREGATED_STORAGE)

namespace godot {

	template<class O, size_t bufferSize = 1, size_t poolSize = POOL_SIZE>
	class ObjectPool {
	private:
		boost::mutex objectPoolMutex;
		boost::simple_segregated_storage<std::size_t> storage;
		std::vector<char>* v;
	public:
		ObjectPool() {
			v = new std::vector<char>(poolSize * bufferSize * sizeof(O));
			storage.add_block(&v->front(), v->size(), bufferSize * sizeof(O));
		};
		~ObjectPool() {};
		O* borrow() {
			objectPoolMutex.lock();
			O* o = static_cast<O*>(storage.malloc());
			objectPoolMutex.unlock();
			return o;
		};
		void ret(O* o) {
			objectPoolMutex.lock();
			storage.free(o);
			objectPoolMutex.unlock();
		};
	};

}

#else

namespace godot {

	template<class O, size_t bufferSize = 1, size_t poolSize = POOL_SIZE>
	class ObjectPool {
		typedef boost::fast_pool_allocator<O, boost::default_user_allocator_new_delete, boost::details::pool::default_mutex> ObjectAllocator;
		typedef boost::singleton_pool<O, sizeof(O) * bufferSize> ObjectAllocatorPool;
	private:
		list<O*, ObjectAllocator>* l;
		boost::mutex mutex;
		semaphore semaphore;
	public:
		ObjectPool() : semaphore(poolSize) {
			l = new list<O*, ObjectAllocator>(poolSize);
			for (int i = 0; i < poolSize; i++) {
				O* o = static_cast<O*>(ObjectAllocatorPool::malloc());
				new (o) O();
				l->push_front(o);
			}
		};
		~ObjectPool() {
			l->clear();
			ObjectAllocatorPool::purge_memory();
		};
		O* borrow() {
			O* o = NULL;
			
			do {
				semaphore.wait();
				mutex.lock();
				if (!l->empty()) {
					o = l->front();
					l->pop_front();
				}
				mutex.unlock();
			} while (!o);
			
			return o;
		};
		void ret(O* o) {
			mutex.lock();
			l->push_front(o);
			mutex.unlock();
			semaphore.signal();
		};
	};

}

#endif

#endif