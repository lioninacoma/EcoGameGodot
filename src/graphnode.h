#ifndef GRAPHNODE_H
#define GRAPHNODE_H

#include <Godot.hpp>
#include <Vector3.hpp>

#include <unordered_set>
#include <unordered_map>

#include "constants.h"
#include "fn.h"
#include "graphedge.h"

using namespace std;

namespace godot {
	class GraphNode {
	private:
		Vector3 point;
		unordered_map<size_t, GraphEdge*> edges;
		size_t hash;
		char voxel;
		boost::shared_timed_mutex EDGES_MUTEX;
	public:
		GraphNode(Vector3 point, char voxel) {
			GraphNode::point = point;
			GraphNode::hash = fn::hash(point);
			GraphNode::voxel = voxel;
		};
		void addEdge(GraphEdge* edge) {
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
		GraphEdge* getEdgeWithNode(size_t nHash) {
			boost::shared_lock<boost::shared_timed_mutex> lock(EDGES_MUTEX);
			auto it = edges.find(nHash);
			if (it == edges.end()) return NULL;
			return it->second;
		};
		GraphNode* getNeighbour(size_t nHash) {
			GraphEdge* edge = getEdgeWithNode(nHash);
			if (!edge) return NULL;
			return (edge->getA()->getHash() != hash) ? edge->getA() : edge->getB();
		};
		unordered_map<size_t, GraphEdge*> getEdges() {
			return edges;
		};
		void lockEdges() {
			EDGES_MUTEX.lock_shared();
		};
		void unlockEdges() {
			EDGES_MUTEX.unlock_shared();
		};
		Vector3 getPoint() {
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