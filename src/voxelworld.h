#ifndef VOXELWORLD_H
#define VOXELWORLD_H

#include <Godot.hpp>
#include <Spatial.hpp>

#include <vector>
#include <iostream>
#include <unordered_map> 

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>

#include "constants.h"
#include "fn.h"

using namespace std;

namespace godot {
	class GraphNode;

	class VoxelWorld : public Spatial {
		GODOT_CLASS(VoxelWorld, Spatial)

	private:
	public:
		static void _register_methods();

		VoxelWorld();
		~VoxelWorld();

		void _init();
		void _notification(const int64_t what);
	};
}

#endif