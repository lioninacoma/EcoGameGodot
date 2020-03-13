#ifndef GRAPHNODE_H
#define GRAPHNODE_H

#include <Godot.hpp>
#include <Node.hpp>
#include <Vector3.hpp>

#include <unordered_set>
#include <unordered_map>
#include <atomic>

#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/function.hpp>

#include "constants.h"
#include "fn.h"

using namespace std;

namespace godot {

	class GraphEdge;

	class GraphNavNode : public Node {
		GODOT_CLASS(GraphNavNode, Node)
	private:
		unordered_map<size_t, std::shared_ptr<GraphEdge>> edges;

		std::shared_ptr<Vector3> point;
		std::shared_ptr<Vector3> gravity;

		std::atomic<size_t> hash;
		std::atomic<char> voxel;
		std::atomic<unsigned char> directionMask;

		boost::shared_mutex EDGES_MUTEX;
		boost::mutex DIRECTION_MASK_MUTEX;
	public:
		static void _register_methods();
		static enum DIRECTION {
			TOP, BOTTOM, WEST, EAST, NORTH, SOUTH
		};
		static Vector3 getDirectionVector(DIRECTION d);

		static DIRECTION getDirectionFromVector(Vector3 v) {
			float x[3] = { v.x, v.y, v.z };
			float y[3] = { 0.0, 0.0, 0.0 };
			float m = 0.0, n;
			int d = 0;
			int sgn = 1;

			for (int i = 0; i < 3; i++) {
				n = abs(x[i]);
				if (n > m) {
					m = n;
					d = i;
					sgn = x[i] >= 0 ? 1 : -1;
				}
			}

			y[d] = 1.0 * sgn;

			Vector3 cardinal = Vector3(y[0], y[1], y[2]);
			DIRECTION dir = static_cast<DIRECTION>(0);

			for (int i = 1; i < 6 && cardinal != getDirectionVector(dir); i++) {
				dir = static_cast<DIRECTION>(i);
			}

			return dir;
		};

		GraphNavNode() : GraphNavNode(Vector3(), 0) {};
		GraphNavNode(Vector3 point, char voxel);
		~GraphNavNode();

		void _init(); // our initializer called by Godot

		void addEdge(std::shared_ptr<GraphEdge> edge);
		void removeEdgeWithNode(size_t nHash);
		void setPoint(Vector3 point);
		void setVoxel(char voxel);
		void determineGravity(Vector3 cog);
		void setDirection(DIRECTION d, bool set);
		void setDirections(unsigned char mask);
		void clearDirections();

		void forEachEdge(std::function<void(std::pair<size_t, std::shared_ptr<GraphEdge>>)> func);
		std::shared_ptr<GraphEdge> getEdgeWithNode(size_t nHash);
		std::shared_ptr<GraphNavNode> getNeighbour(size_t nHash);
		std::shared_ptr<Vector3> getPoint();
		std::shared_ptr<Vector3> getGravity();
		Vector3 getPointU();
		Vector3 getGravityU();
		size_t getHash();
		char getVoxel();
		int getAmountDirections();
		vector<int> getDirections();
		PoolVector3Array getDirectionVectors();
		bool isDirectionSet(DIRECTION d);
		bool isWalkable();

		bool operator == (const GraphNavNode& o) const {
			return hash == o.hash;
		};
	};

}

#endif