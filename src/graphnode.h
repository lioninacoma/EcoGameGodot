#ifndef GRAPHNODE_H
#define GRAPHNODE_H

#include <Godot.hpp>
#include <Vector3.hpp>

#include <unordered_set>
#include <unordered_map>

#include <boost/atomic.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/function.hpp>

#include "constants.h"
#include "fn.h"
#include "graphedge.h"

using namespace std;

namespace godot {
	class GraphNode {
	private:
		Vector3 point;
		unordered_map<size_t, boost::shared_ptr<GraphEdge>> edges;
		boost::atomic<size_t> hash;
		boost::atomic<char> voxel;
		boost::shared_timed_mutex EDGES_MUTEX;
		boost::shared_timed_mutex POINT_MUTEX;
	public:
		GraphNode(Vector3 point, char voxel) {
			GraphNode::point = point;
			GraphNode::hash = fn::hash(point);
			GraphNode::voxel = voxel;
		};
		void addEdge(boost::shared_ptr<GraphEdge> edge) {
			boost::unique_lock<boost::shared_timed_mutex> lock(EDGES_MUTEX);
			size_t nHash = (edge->getA()->getHash() != hash) ? edge->getA()->getHash() : edge->getB()->getHash();
			edges[nHash] = edge;
		};
		void removeEdgeWithNode(size_t nHash) {
			boost::unique_lock<boost::shared_timed_mutex> lock(EDGES_MUTEX);
			auto it = edges.find(nHash);
			if (it == edges.end()) return;
			edges.erase(nHash);
		};
		boost::shared_ptr<GraphEdge> getEdgeWithNode(size_t nHash) {
			boost::shared_lock<boost::shared_timed_mutex> lock(EDGES_MUTEX);
			auto it = edges.find(nHash);
			if (it == edges.end()) return NULL;
			return it->second;
		};
		boost::shared_ptr<GraphNode> getNeighbour(size_t nHash) {
			boost::shared_ptr<GraphEdge> edge = getEdgeWithNode(nHash);
			if (!edge) return NULL;
			return (edge->getA()->getHash() != hash) ? edge->getA() : edge->getB();
		};
		void forEachEdge(std::function<void(std::pair<size_t, boost::shared_ptr<GraphEdge>>)> func) {
			boost::shared_lock<boost::shared_timed_mutex> lock(EDGES_MUTEX);
			std::for_each(edges.begin(), edges.end(), func);
		};
		Vector3 getPoint() {
			boost::shared_lock<boost::shared_timed_mutex> lock(POINT_MUTEX);
			return point;
		};
		size_t getHash() {
			return hash;
		};
		char getVoxel() {
			return voxel;
		};
		bool operator == (const GraphNode& o) const {
			return hash == o.hash;
		};
	};
}

#endif