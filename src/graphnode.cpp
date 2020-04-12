#include "graphnode.h"
#include "graphedge.h"

using namespace godot;

GraphNode::GraphNode(Vector3 point, char voxel) {
	GraphNode::point = std::make_shared<Vector3>(point);
	GraphNode::gravity = std::make_shared<Vector3>(0, -9.8, 0);
	GraphNode::hash = fn::hash(point);
	GraphNode::voxel = voxel;
}

GraphNode::~GraphNode() {
	point.reset();
	gravity.reset();
}

void GraphNode::addEdge(std::shared_ptr<GraphEdge> edge) {
	boost::unique_lock<boost::shared_mutex> lock(EDGES_MUTEX);
	auto neighbour = edge->getA();
	if (neighbour->getHash() == hash)
		neighbour = edge->getB();
	size_t nHash = neighbour->getHash();
	auto it = edges.find(nHash);
	if (it == edges.end()) {
		edges.emplace(nHash, edge);
	}
}

void GraphNode::setPoint(Vector3 point) {
	GraphNode::point = std::make_shared<Vector3>(point);
	GraphNode::hash = fn::hash(point);
}

void GraphNode::setVoxel(char voxel) {
	GraphNode::voxel = voxel;
}

void GraphNode::determineGravity(Vector3 cog) {
	Vector3 g = cog - *point;
	g.normalize();
	g *= 9.8;
	gravity = std::make_shared<Vector3>(g);
}

void GraphNode::removeEdgeWithNode(size_t nHash) {
	boost::unique_lock<boost::shared_mutex> lock(EDGES_MUTEX);
	auto it = edges.find(nHash);
	if (it == edges.end()) return;
	auto edge = it->second;
	edges.erase(nHash);
	edge.reset();
}

void GraphNode::clearEdges() {
	boost::unique_lock<boost::shared_mutex> lock(EDGES_MUTEX);
	while (!edges.empty()) {
		auto it = edges.begin();
		size_t nHash = it->first;
		auto edge = it->second;

		auto neighbour = edge->getA();
		if (neighbour->getHash() == hash)
			neighbour = edge->getB();

		neighbour->removeEdgeWithNode(hash);
		edges.erase(nHash);
		edge.reset();
		if (edge.use_count() > 0 || edge)
			cout << "edge not deleted" << endl;
	}
}

void GraphNode::forEachEdge(std::function<void(std::pair<size_t, std::shared_ptr<GraphEdge>>)> func) {
	boost::shared_lock<boost::shared_mutex> lock(EDGES_MUTEX);
	std::for_each(edges.begin(), edges.end(), func);
}

std::shared_ptr<GraphEdge> GraphNode::getEdgeWithNode(size_t nHash) {
	boost::shared_lock<boost::shared_mutex> lock(EDGES_MUTEX);
	auto it = edges.find(nHash);
	if (it == edges.end()) return NULL;
	return it->second;
}

std::shared_ptr<GraphNode> GraphNode::getNeighbour(size_t nHash) {
	std::shared_ptr<GraphEdge> edge = getEdgeWithNode(nHash);
	if (!edge) return NULL;
	auto neighbour = edge->getA();
	if (neighbour->getHash() == hash)
		neighbour = edge->getB();
	return neighbour;
}

std::shared_ptr<Vector3> GraphNode::getPoint() {
	return point;
}

std::shared_ptr<Vector3> GraphNode::getGravity() {
	return gravity;
}

Vector3 GraphNode::getPointU() {
	return fn::unreference(point);
}

Vector3 GraphNode::getGravityU() {
	return fn::unreference(gravity);
}

size_t GraphNode::getHash() {
	return hash;
}

char GraphNode::getVoxel() {
	return voxel;
}

bool GraphNode::isWalkable() {
	return true;
}

int GraphNode::getAmountEdges() {
	boost::shared_lock<boost::shared_mutex> lock(EDGES_MUTEX);
	return edges.size();
}