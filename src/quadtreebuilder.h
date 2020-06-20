#ifndef QUADTREEBUILDER_H
#define QUADTREEBUILDER_H

#include <Spatial.hpp>
#include <String.hpp>
#include <Array.hpp>
#include <MeshInstance.hpp>
#include <ArrayMesh.hpp>

#include <deque>
#include <unordered_set>
#include <vector>
#include <iostream>

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include "constants.h"
#include "quadtree.h"
#include "objectpool.h"
#include "fn.h"

using namespace std;
namespace bpt = boost::posix_time;

#define QUADTREEBUILDER_VERTEX_SIZE 3
#define QUADTREEBUILDER_FACE_SIZE 3
#define QUADTREEBUILDER_MAX_VERTICES pow(4, QUADTREE_LEVEL) * 9 * 2
#define QUADTREEBUILDER_MAX_FACES QUADTREEBUILDER_MAX_VERTICES

namespace godot {

	class VoxelWorld;

	class QuadTreeBuilder {

	private:
		struct queueEntry {
			quadsquare* quadtree;
			Vector3 cameraPosition;
		};

		void processQueue();
		void queueQuadTree(quadsquare* quadtree, Vector3 cameraPosition);
		void buildQuadTree(quadsquare* quadtree, Vector3 cameraPosition);

		std::shared_ptr<VoxelWorld> world;
		std::unique_ptr<boost::thread> queueThread;
		deque<queueEntry> buildQueue;
		unordered_set<size_t> inque;

		boost::mutex BUILD_QUEUE_MUTEX;
		boost::mutex BUILD_QUEUE_WAIT;
		boost::mutex BUILD_MESH_MUTEX;
		boost::condition_variable BUILD_QUEUE_CV;
		std::atomic<bool> threadStarted = false;
	public:
		QuadTreeBuilder(std::shared_ptr<VoxelWorld> world);
		~QuadTreeBuilder() {
			queueThread->interrupt();
			queueThread->join();
		};
		void build(quadsquare* quadtree, Vector3 cameraPosition);
	};

}

#endif