#ifndef VOXELDATA_H
#define VOXELDATA_H

#include <Godot.hpp>

#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/lock_types.hpp>
#include <boost/atomic.hpp>

#include "constants.h"
#include "fn.h"

using namespace std;

namespace godot {
	class VoxelData {
	private:
		char* volume;
		boost::atomic<int> width, height, depth;
		boost::shared_timed_mutex VOLUME_MUTEX;
	public:
		VoxelData(int width, int height, int depth) {
			VoxelData::width = width;
			VoxelData::height = height;
			VoxelData::depth = depth;
			VoxelData::volume = new char[width * height * depth];
			memset(volume, 0, width * height * depth * sizeof(*volume));
		};
		~VoxelData() {
			delete[] volume;
		};
		int get(int x, int y, int z) {
			boost::shared_lock<boost::shared_timed_mutex> lock(VOLUME_MUTEX);
			return volume[fn::fi3(x, y, z, width, height)];
		};
		void set(int x, int y, int z, int v) {
			boost::unique_lock<boost::shared_timed_mutex> lock(VOLUME_MUTEX);
			volume[fn::fi3(x, y, z, width, height)] = v;
		};
		int getWidth() {
			return width;
		};
		int getHeight() {
			return height;
		};
		int getDepth() {
			return depth;
		};
	};
}

#endif