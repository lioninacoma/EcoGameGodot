#ifndef OBJECTPOOL_H
#define OBJECTPOOL_H

#define SEGREGATED_STORAGE

#include <boost/pool/pool_alloc.hpp>
#include <boost/pool/simple_segregated_storage.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_types.hpp>

#include <vector>
#include <list>
#include <cstddef>
#include <cassert>
#include <iostream>

#include "constants.h"
#include "semaphore.h"

using namespace std;

#if defined(SEGREGATED_STORAGE)

namespace godot {

	template<class O, size_t bufferSize = 1, size_t poolSize = POOL_SIZE>
	class ObjectPool {
	private:
		boost::mutex mutex;
		boost::simple_segregated_storage<std::size_t> storage;
		std::vector<char>* v;
		semaphore semaphore;
	private:
		O* doBorrow() {
			boost::unique_lock<boost::mutex> lock(mutex);
			return static_cast<O*>(storage.malloc());
		}

		void doRet(O* o) {
			boost::unique_lock<boost::mutex> lock(mutex);
			storage.free(o);
		}
	public:
		ObjectPool() : semaphore(poolSize) {
			v = new std::vector<char>(poolSize * bufferSize * sizeof(O));
			storage.add_block(&v->front(), v->size(), bufferSize * sizeof(O));
		};
		~ObjectPool() {};

		O* borrow() {
			semaphore.wait();
			O* o = doBorrow();
			return o;
		};

		O** borrow(int count) {
			int i;

			semaphore.wait(count);
			O** oList = new O * [count];

			for (i = 0; i < count; i++) {
				oList[i] = doBorrow();
			}

			return oList;
		}

		void ret(O* o) {
			doRet(o);
			semaphore.signal();
		};

		void ret(O** oList, int count) {
			int i;
			for (i = 0; i < count; i++) {
				doRet(oList[i]);
			}

			semaphore.signal(count);
		}
	};

}

#else

namespace godot {

	template<class O, size_t bufferSize = 1, size_t poolSize = POOL_SIZE>
	class ObjectPool {
		typedef boost::singleton_pool<O, sizeof(O) * bufferSize> ObjectAllocatorPool;
	private:
		list<O*>* l;
		boost::mutex mutex;
		semaphore semaphore;
	public:
		ObjectPool() : semaphore(poolSize) {
			l = new list<O*>(poolSize);
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