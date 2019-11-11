#ifndef OBJECTPOOL_H
#define OBJECTPOOL_H

#include <boost/thread/mutex.hpp>
#include <boost/pool/simple_segregated_storage.hpp>
#include <vector>
#include <cstddef>
#include <cassert>
#include <iostream>

#include "constants.h"

using namespace std;

namespace godot {

	template<class O, size_t bufferSize = BUFFER_SIZE, size_t poolSize = POOL_SIZE>
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

#endif