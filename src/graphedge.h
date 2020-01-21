#ifndef GRAPHEDGE_H
#define GRAPHEDGE_H

#include <Godot.hpp>
#include <Vector3.hpp>

#include <unordered_map>

#include "constants.h"
#include "fn.h"

using namespace std;

namespace godot {
	class GraphNode;

	class GraphEdge {
	private:
		GraphNode* a;
		GraphNode* b;
		float cost;
	public:
		GraphEdge(GraphNode* a, GraphNode* b, float cost) {
			GraphEdge::a = a;
			GraphEdge::b = b;
			GraphEdge::cost = cost;
		};
		GraphNode* getA() {
			return a;
		};
		GraphNode* getB() {
			return b;
		};
		float getCost() {
			return cost;
		};
	};
}

#endif