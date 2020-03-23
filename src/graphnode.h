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

		boost::shared_mutex EDGES_MUTEX;
	public:
		static void _register_methods();

		GraphNavNode() : GraphNavNode(Vector3(), 0) {};
		GraphNavNode(Vector3 point, char voxel);
		~GraphNavNode();

		void _init(); // our initializer called by Godot

		void addEdge(std::shared_ptr<GraphEdge> edge);
		void removeEdgeWithNode(size_t nHash);
		void setPoint(Vector3 point);
		void setVoxel(char voxel);
		void determineGravity(Vector3 cog);

		void forEachEdge(std::function<void(std::pair<size_t, std::shared_ptr<GraphEdge>>)> func);
		std::shared_ptr<GraphEdge> getEdgeWithNode(size_t nHash);
		std::shared_ptr<GraphNavNode> getNeighbour(size_t nHash);
		std::shared_ptr<Vector3> getPoint();
		std::shared_ptr<Vector3> getGravity();
		Vector3 getPointU();
		Vector3 getGravityU();
		size_t getHash();
		char getVoxel();
		bool isWalkable();

		bool operator == (const GraphNavNode& o) const {
			return hash == o.hash;
		};
	};

}

#endif