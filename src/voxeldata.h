#ifndef VOXELDATA_H
#define VOXELDATA_H

#include <Godot.hpp>

#include <atomic>

#include <boost/thread/mutex.hpp>

#include "constants.h"
#include "fn.h"

using namespace std;

namespace godot {
	struct VoxelPlane {
		VoxelPlane() : VoxelPlane(100000.f, godot::Vector3()) {}
		VoxelPlane(const float _dist, const godot::Vector3& _normal) : dist(_dist), normal(_normal) {}
		float dist;
		godot::Vector3 normal;
	};

	class VoxelData {
	private:
		VoxelPlane* volume;
		int width, height, depth;
		boost::mutex VOLUME_MUTEX;
	public:
		VoxelData(int width, int height, int depth) {
			VoxelData::width = width;
			VoxelData::height = height;
			VoxelData::depth = depth;
			VoxelData::volume = new VoxelPlane[width * height * depth];
			//memset(volume, 0, width * height * depth * sizeof(*volume));
			for (int i = 0; i < width * height * depth; i++)
				volume[i] = VoxelPlane();
		};
		~VoxelData() {
			delete[] volume;
		};
		VoxelPlane get(int x, int y, int z) {
			if (x < 0 || x >= width) return VoxelPlane();
			if (y < 0 || y >= height) return VoxelPlane();
			if (z < 0 || z >= depth) return VoxelPlane();

			boost::unique_lock<boost::mutex> lock(VOLUME_MUTEX);
			return volume[fn::fi3(x, y, z, width, height)];
		};
		VoxelPlane get(int i) {
			boost::unique_lock<boost::mutex> lock(VOLUME_MUTEX);
			int len = width * height * depth;
			if (i < 0 || i >= len) return VoxelPlane();
			return volume[i];
		};
		void set(int x, int y, int z, VoxelPlane data) {
			if (x < 0 || x >= width) return;
			if (y < 0 || y >= height) return;
			if (z < 0 || z >= depth) return;

			boost::unique_lock<boost::mutex> lock(VOLUME_MUTEX);
			int i = fn::fi3(x, y, z, width, height);
			volume[i] = data;
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