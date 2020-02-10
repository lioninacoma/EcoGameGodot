#ifndef GRAPHEDGE_H
#define GRAPHEDGE_H

#include <Godot.hpp>
#include <Vector3.hpp>

#include <unordered_map>

#include <boost/atomic.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>

#include "constants.h"
#include "fn.h"

using namespace std;

namespace godot {
	class GraphNode;

	class GraphEdge {
	private:
		boost::shared_ptr<GraphNode> a;
		boost::shared_ptr<GraphNode> b;
		boost::atomic<float> cost;
	public:
		GraphEdge(boost::shared_ptr<GraphNode> a, boost::shared_ptr<GraphNode> b, float cost) {
			GraphEdge::a = a;
			GraphEdge::b = b;
			GraphEdge::cost = cost;
		};
		~GraphEdge() {
			a.reset();
			b.reset();
		};
		boost::shared_ptr<GraphNode> getA() {
			return a;
		};
		boost::shared_ptr<GraphNode> getB() {
			return b;
		};
		float getCost() {
			return cost;
		};
	};
}

#endif