#ifndef GRAPHNODE_H
#define GRAPHNODE_H

#include <Godot.hpp>
#include <Vector3.hpp>

#include <unordered_set>
#include <unordered_map>
#include <atomic>

#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/function.hpp>

#include "constants.h"
#include "fn.h"
#include "graphedge.h"

using namespace std;

namespace godot {
	class GraphNode {
	private:
		std::shared_ptr<Vector3> point;
		unordered_map<size_t, std::shared_ptr<GraphEdge>> edges;
		std::atomic<size_t> hash;
		std::atomic<char> voxel;
		boost::shared_mutex EDGES_MUTEX;
	public:
		GraphNode(Vector3 point, char voxel) {
			GraphNode::point = std::shared_ptr<Vector3>(new Vector3(point));
			GraphNode::hash = fn::hash(point);
			GraphNode::voxel = voxel;
		};
		~GraphNode() {
			point.reset();
			for (auto edge : edges)
				edge.second.reset();
			edges.clear();
		};
		void addEdge(std::shared_ptr<GraphEdge> edge) {
			boost::unique_lock<boost::shared_mutex> lock(EDGES_MUTEX);
			size_t nHash = (edge->getA()->getHash() != hash) ? edge->getA()->getHash() : edge->getB()->getHash();
			edges[nHash] = edge;
		};
		void removeEdgeWithNode(size_t nHash) {
			boost::unique_lock<boost::shared_mutex> lock(EDGES_MUTEX);
			auto it = edges.find(nHash);
			if (it == edges.end()) return;
			edges.erase(nHash);
		};
		std::shared_ptr<GraphEdge> getEdgeWithNode(size_t nHash) {
			boost::shared_lock<boost::shared_mutex> lock(EDGES_MUTEX);
			auto it = edges.find(nHash);
			if (it == edges.end()) return NULL;
			return it->second;
		};
		std::shared_ptr<GraphNode> getNeighbour(size_t nHash) {
			std::shared_ptr<GraphEdge> edge = getEdgeWithNode(nHash);
			if (!edge) return NULL;
			return (edge->getA()->getHash() != hash) ? edge->getA() : edge->getB();
		};
		void forEachEdge(std::function<void(std::pair<size_t, std::shared_ptr<GraphEdge>>)> func) {
			boost::shared_lock<boost::shared_mutex> lock(EDGES_MUTEX);
			std::for_each(edges.begin(), edges.end(), func);
		};
		std::shared_ptr<Vector3> getPoint() {
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