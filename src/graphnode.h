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

	class GraphNode {
	private:
		unordered_map<size_t, std::shared_ptr<GraphEdge>> edges;

		std::shared_ptr<Vector3> point;
		std::shared_ptr<Vector3> gravity;

		std::atomic<size_t> hash;
		std::atomic<char> voxel;

		boost::shared_mutex EDGES_MUTEX;
	public:
		GraphNode() : GraphNode(Vector3(), 0) {};
		GraphNode(Vector3 point, char voxel);
		~GraphNode();

		void addEdge(std::shared_ptr<GraphEdge> edge);
		void removeEdgeWithNode(size_t nHash);
		void setPoint(Vector3 point);
		void setVoxel(char voxel);
		void determineGravity(Vector3 cog);
		void clearEdges();

		void forEachEdge(std::function<void(std::pair<size_t, std::shared_ptr<GraphEdge>>)> func);
		std::shared_ptr<GraphEdge> getEdgeWithNode(size_t nHash);
		std::shared_ptr<GraphNode> getNeighbour(size_t nHash);
		std::shared_ptr<Vector3> getPoint();
		std::shared_ptr<Vector3> getGravity();
		Vector3 getPointU();
		Vector3 getGravityU();
		size_t getHash();
		char getVoxel();
		bool isWalkable();
		int getAmountEdges();

		bool operator == (const GraphNode& o) const {
			return hash == o.hash;
		};
	};

}

#endif