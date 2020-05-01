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
		std::shared_ptr<Vector3> normal;

		std::atomic<size_t> hash;

		boost::shared_mutex EDGES_MUTEX;
	public:
		GraphNode() : GraphNode(Vector3(), Vector3()) {};
		GraphNode(Vector3 point, Vector3 normal);
		~GraphNode();

		void addEdge(std::shared_ptr<GraphEdge> edge);
		void removeEdgeWithNode(size_t nHash);
		void setPoint(Vector3 point);
		void clearEdges();

		void forEachEdge(std::function<void(std::pair<size_t, std::shared_ptr<GraphEdge>>)> func);
		std::shared_ptr<GraphEdge> getEdgeWithNode(size_t nHash);
		std::shared_ptr<GraphNode> getNeighbour(size_t nHash);
		std::shared_ptr<Vector3> getPoint();
		Vector3 getPointU();
		std::shared_ptr<Vector3> getNormal();
		Vector3 getNormalU();
		size_t getHash();
		int getAmountEdges();

		bool operator == (const GraphNode& o) const {
			return hash == o.hash;
		};
	};

}

#endif