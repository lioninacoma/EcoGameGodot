#ifndef GRAPHNODE_H
#define GRAPHNODE_H

#include <Godot.hpp>
#include <Vector3.hpp>

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
		unordered_map<char, unordered_map<size_t, GraphNode*>*> reachableVoxels;
		size_t hash;
		int type;
		char voxel;
	public:
		GraphNode(Vector3 point, char voxel) {
			GraphNode::point = point;
			GraphNode::hash = fn::hash(point);
			GraphNode::type = 0;
			GraphNode::voxel = voxel;
		};
		void addReachableVoxel(GraphNode* node) {
			auto it = reachableVoxels.find(node->getVoxel());
			unordered_map<size_t, GraphNode*>* map;

			if (it == reachableVoxels.end()) {
				map = new unordered_map<size_t, GraphNode*>();
				reachableVoxels.emplace(node->getVoxel(), map);
			}
			else {
				map = it->second;
			}

			map->emplace(node->getHash(), node);
		};
		void removeReachableVoxel(GraphNode* node) {
			auto it = reachableVoxels.find(node->getVoxel());
			unordered_map<size_t, GraphNode*>* map;

			if (it != reachableVoxels.end()) {
				map = it->second;
				map->erase(node->getHash());

				if (map->empty()) {
					reachableVoxels.erase(node->getVoxel());
				}
			}
		};
		void addEdge(GraphEdge* edge) {
			size_t nHash = (edge->getA()->getHash() != hash) ? edge->getA()->getHash() : edge->getB()->getHash();
			edges[nHash] = edge;
		};
		void removeEdgeWithNode(size_t nHash) {
			auto it = edges.find(nHash);
			if (it == edges.end()) return;
			edges.erase(nHash);
		};
		void setType(int type) {
			GraphNode::type = type;
		}
		GraphEdge* getEdgeWithNode(size_t nHash) {
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
		Vector3 getPoint() {
			return point;
		};
		size_t getHash() {
			return hash;
		};
		unordered_map<size_t, GraphNode*>* getReachableVoxelsOfType(char voxel) {
			auto it = reachableVoxels.find(voxel);
			return (it == reachableVoxels.end()) ? NULL : it->second;
		};
		unordered_map<char, unordered_map<size_t, GraphNode*>*> getReachableVoxels() {
			return reachableVoxels;
		};
		int getType() {
			return type;
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