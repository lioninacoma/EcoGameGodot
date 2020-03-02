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
		static Vector3 getDirectionVector(GraphNavNode::DIRECTION d) {
			switch (d) {
			case TOP:
				return Vector3(0, 1, 0);
			case BOTTOM:
				return Vector3(0, -1, 0);
			case WEST:
				return Vector3(-1, 0, 0);
			case EAST:
				return Vector3(1, 0, 0);
			case NORTH:
				return Vector3(0, 0, 1);
			case SOUTH:
				return Vector3(0, 0, -1);
			}
		}

		GraphNavNode() : GraphNavNode(Vector3(), 0) {};
		GraphNavNode(Vector3 point, char voxel);
		~GraphNavNode();

		void _init(); // our initializer called by Godot

		void addEdge(std::shared_ptr<GraphEdge> edge);
		void removeEdgeWithNode(size_t nHash);
		void setPoint(Vector3 point);
		void setVoxel(char voxel);
		void determineGravity(Vector3 cog);
		void setDirection(GraphNavNode::DIRECTION d, bool set) {
			boost::unique_lock<boost::mutex> lock(DIRECTION_MASK_MUTEX);
			if (set) directionMask |= 1UL << d;
			else directionMask &= ~(1UL << d);
		}
		void setDirections(unsigned char mask) {
			boost::unique_lock<boost::mutex> lock(DIRECTION_MASK_MUTEX);
			directionMask |= mask;
		}
		void clearDirections() {
			boost::unique_lock<boost::mutex> lock(DIRECTION_MASK_MUTEX);
			directionMask &= 0;
		}

		void forEachEdge(std::function<void(std::pair<size_t, std::shared_ptr<GraphEdge>>)> func);
		std::shared_ptr<GraphEdge> getEdgeWithNode(size_t nHash);
		std::shared_ptr<GraphNavNode> getNeighbour(size_t nHash);
		std::shared_ptr<Vector3> getPoint();
		std::shared_ptr<Vector3> getGravity();
		Vector3 getPointU();
		Vector3 getGravityU();
		size_t getHash();
		char getVoxel();
		int getAmountDirections() {
			boost::unique_lock<boost::mutex> lock(DIRECTION_MASK_MUTEX);
			int count = 0;
			unsigned char n = directionMask;
			while (n) {
				count += n & 1;
				n >>= 1;
			}
			return count;
		}
		bool isDirectionSet(GraphNavNode::DIRECTION d) {
			boost::unique_lock<boost::mutex> lock(DIRECTION_MASK_MUTEX);
			return (bool)((directionMask >> d) & 1U);
		}
		unsigned char getDirectionMask() {
			boost::unique_lock<boost::mutex> lock(DIRECTION_MASK_MUTEX);
			return directionMask;
		}

		bool operator == (const GraphNavNode& o) const {
			return hash == o.hash;
		};
	};

}

#endif