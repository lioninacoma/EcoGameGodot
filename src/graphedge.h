#ifndef GRAPHEDGE_H
#define GRAPHEDGE_H

#include <Godot.hpp>
#include <Vector3.hpp>

#include <unordered_map>
#include <atomic>

#include "constants.h"
#include "fn.h"

using namespace std;

namespace godot {
	class GraphNavNode;

	class GraphEdge {
	private:
		std::shared_ptr<GraphNavNode> a;
		std::shared_ptr<GraphNavNode> b;
		std::atomic<float> cost;
	public:
		GraphEdge(std::shared_ptr<GraphNavNode> a, std::shared_ptr<GraphNavNode> b, float cost) {
			GraphEdge::a = a;
			GraphEdge::b = b;
			GraphEdge::cost = cost;
		};
		~GraphEdge() {
			//a.reset();
			//b.reset();
		};
		std::shared_ptr<GraphNavNode> getA() {
			return a;
		};
		std::shared_ptr<GraphNavNode> getB() {
			return b;
		};
		float getCost() {
			return cost;
		};
	};
}

#endif