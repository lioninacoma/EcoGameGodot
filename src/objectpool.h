#ifndef OBJECTPOOL_H
#define OBJECTPOOL_H

#include <vector>
#include <list>
#include <cstddef>
#include <cassert>
#include <iostream>

#include <boost/thread/mutex.hpp>
#include <boost/pool/pool_alloc.hpp>
#include <boost/pool/simple_segregated_storage.hpp>

#include "constants.h"
#include "semaphore.h"

using namespace std;

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

#endif