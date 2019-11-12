#ifndef OBJECTPOOL_H
#define OBJECTPOOL_H
#define SEGREGATED_STORAGE

#include <boost/thread/mutex.hpp>
#include <boost/pool/pool_alloc.hpp>
#include <boost/pool/simple_segregated_storage.hpp>
#include <vector>
#include <cstddef>
#include <cassert>
#include <iostream>

#include "constants.h"

using namespace std;

#if defined(SEGREGATED_STORAGE)

namespace godot {

	template<class O, size_t bufferSize = 1, size_t poolSize = POOL_SIZE>
	class ObjectPool {
	private:
		boost::mutex mutex;
		boost::simple_segregated_storage<std::size_t> storage;
		std::vector<char>* v;
	public:
		ObjectPool() {
			v = new std::vector<char>(poolSize * bufferSize * sizeof(O));
			storage.add_block(&v->front(), v->size(), bufferSize * sizeof(O));
		};
		~ObjectPool() {};
		O* borrow() {
			mutex.lock();
			O* o = static_cast<O*>(storage.malloc());
			mutex.unlock();
			return o;
		};
		void ret(O* o) {
			mutex.lock();
			storage.free(o);
			mutex.unlock();
		};
	};

}

#else

namespace godot {

	template<class O, size_t poolSize = POOL_SIZE, size_t bufferSize = BUFFER_SIZE>
	class ObjectPool {
		typedef boost::fast_pool_allocator<O, boost::default_user_allocator_new_delete, boost::details::pool::default_mutex> ObjectAllocator;
		typedef boost::singleton_pool<boost::fast_pool_allocator_tag, sizeof(O) * bufferSize> ObjectAllocatorPool;
		list<O*, ObjectAllocator>* l;
		boost::mutex mutex;
	public:
		ObjectPool() {
			l = new list<O*, ObjectAllocator>(poolSize);
			for (int i = 0; i < poolSize; i++) {
				l->push_front(static_cast<O*>(ObjectAllocatorPool::malloc()));
			}
		};
		~ObjectPool() {
			l->clear();
			ObjectAllocatorPool::purge_memory();
		};
		O* borrow() {
			mutex.lock();
			O* o = l->front();
			l->pop_front();
			mutex.unlock();
			return o;
		};
		void ret(O* o) {
			mutex.lock();
			l->push_front(o);
			mutex.unlock();
		};
	};

}

#endif

#endif