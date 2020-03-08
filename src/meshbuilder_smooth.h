#ifndef MeshBuilder_Smooth_SMOOTH_H
#define MeshBuilder_Smooth_SMOOTH_H

#include <String.hpp>
#include <Vector3.hpp>

#include <iostream>

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/thread/thread.hpp>

#include "constants.h"
#include "objectpool.h"
#include "chunk.h"
#include "voxelassetmanager.h"
#include "voxelasset.h"

namespace bpt = boost::posix_time;
using namespace std;

namespace godot {

	class GraphNavNode;

	class MeshBuilder_Smooth {
	private:
		int* cube_edges = new int[24];
		int* edge_table = new int[256];

		void initCubeEdges();
		void initEdgeTable();
	public:
		MeshBuilder_Smooth();
		~MeshBuilder_Smooth();

		Array buildVertices(std::shared_ptr<Chunk> chunk);
	};
}

#endif