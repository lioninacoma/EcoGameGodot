#ifndef VOXELWORLD_H
#define VOXELWORLD_H

#include <Godot.hpp>
#include <Spatial.hpp>
#include <Mesh.hpp>

#include <vector>
#include <iostream>
#include <unordered_map> 

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>

#include "constants.h"
#include "fn.h"
#include "octree.h"

using namespace std;

namespace godot {
	class VoxelWorld : public Spatial {
		GODOT_CLASS(VoxelWorld, Spatial)

	private:
		std::shared_ptr<VoxelWorld> self;
		std::shared_ptr<OctreeNode> root;
		vector<std::shared_ptr<OctreeNode>> nodes;

		void buildMesh(std::shared_ptr<OctreeNode> node);
	public:
		static void _register_methods();

		VoxelWorld() : root(nullptr) {
			self = shared_ptr<VoxelWorld>(this);
		}
		~VoxelWorld() {

		}

		void _init();
		void _notification(const int64_t what);
		void build();
		void update(Vector3 camera);
	};
}

#endif