#ifndef VOXELDATA_H
#define VOXELDATA_H

#include <Godot.hpp>

#include <atomic>

#include <boost/thread/mutex.hpp>

#include "constants.h"
#include "fn.h"

using namespace std;

namespace godot {
	class VoxelData {
	private:
		float* volume;
		std::atomic<int> width, height, depth;
		boost::mutex VOLUME_MUTEX;
	public:
		VoxelData(int width, int height, int depth) {
			VoxelData::width = width;
			VoxelData::height = height;
			VoxelData::depth = depth;
			VoxelData::volume = new float[width * height * depth];
			memset(volume, 0, width * height * depth * sizeof(*volume));
		};
		~VoxelData() {
			delete[] volume;
		};
		float get(int x, int y, int z) {
			if (x < 0 || x >= width) return 0;
			if (y < 0 || y >= height) return 0;
			if (z < 0 || z >= depth) return 0;

			boost::unique_lock<boost::mutex> lock(VOLUME_MUTEX);
			return volume[fn::fi3(x, y, z, width, height)];
		};
		float get(int i) {
			boost::unique_lock<boost::mutex> lock(VOLUME_MUTEX);
			int len = width * height * depth;
			if (i < 0 || i >= len) return 0;
			return volume[i];
		};
		void set(int x, int y, int z, float v) {
			if (x < 0 || x >= width) return;
			if (y < 0 || y >= height) return;
			if (z < 0 || z >= depth) return;

			boost::unique_lock<boost::mutex> lock(VOLUME_MUTEX);
			volume[fn::fi3(x, y, z, width, height)] += v;
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