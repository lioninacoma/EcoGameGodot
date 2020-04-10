#include "graphnode.h"
#include "graphedge.h"

using namespace godot;

void GraphNavNode::_register_methods() {
	register_method("getPoint", &GraphNavNode::getPointU);
	register_method("getVoxel", &GraphNavNode::getVoxel);
	register_method("getGravity", &GraphNavNode::getGravityU);
}

GraphNavNode::GraphNavNode(Vector3 point, char voxel) {
	GraphNavNode::point = std::make_shared<Vector3>(point);
	GraphNavNode::gravity = std::make_shared<Vector3>(0, -9.8, 0);
	GraphNavNode::hash = fn::hash(point);
	GraphNavNode::voxel = voxel;
}

GraphNavNode::~GraphNavNode() {
	point.reset();
	gravity.reset();
}

void GraphNavNode::_init() {

}

void GraphNavNode::addEdge(std::shared_ptr<GraphEdge> edge) {
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

void GraphNavNode::removeEdgeWithNode(size_t nHash) {
	boost::unique_lock<boost::shared_mutex> lock(EDGES_MUTEX);
	auto it = edges.find(nHash);
	if (it == edges.end()) return;
	it->second.reset();
	edges.erase(nHash);
}

void GraphNavNode::setPoint(Vector3 point) {
	GraphNavNode::point = std::make_shared<Vector3>(point);
	GraphNavNode::hash = fn::hash(point);
}

void GraphNavNode::setVoxel(char voxel) {
	GraphNavNode::voxel = voxel;
}

void GraphNavNode::determineGravity(Vector3 cog) {
	Vector3 g = cog - *point;
	g.normalize();
	g *= 9.8;
	gravity = std::make_shared<Vector3>(g);
}

void GraphNavNode::clearEdges() {
	boost::unique_lock<boost::shared_mutex> lock(EDGES_MUTEX);
	for (auto edge : edges) {
		auto neighbour = edge.second->getA();
		if (neighbour->getHash() == hash)
			neighbour = edge.second->getB();
		neighbour->removeEdgeWithNode(hash);
		edge.second.reset();
		if (edge.second.use_count() > 0 || edge.second)
			cout << "edge not deleted" << endl;
	}
	edges.clear();
}

void GraphNavNode::forEachEdge(std::function<void(std::pair<size_t, std::shared_ptr<GraphEdge>>)> func) {
	boost::shared_lock<boost::shared_mutex> lock(EDGES_MUTEX);
	std::for_each(edges.begin(), edges.end(), func);
}

std::shared_ptr<GraphEdge> GraphNavNode::getEdgeWithNode(size_t nHash) {
	boost::shared_lock<boost::shared_mutex> lock(EDGES_MUTEX);
	auto it = edges.find(nHash);
	if (it == edges.end()) return NULL;
	return it->second;
}

std::shared_ptr<GraphNavNode> GraphNavNode::getNeighbour(size_t nHash) {
	std::shared_ptr<GraphEdge> edge = getEdgeWithNode(nHash);
	if (!edge) return NULL;
	auto neighbour = edge->getA();
	if (neighbour->getHash() == hash)
		neighbour = edge->getB();
	return neighbour;
}

std::shared_ptr<Vector3> GraphNavNode::getPoint() {
	return point;
}

std::shared_ptr<Vector3> GraphNavNode::getGravity() {
	return gravity;
}

Vector3 GraphNavNode::getPointU() {
	return fn::unreference(point);
}

Vector3 GraphNavNode::getGravityU() {
	return fn::unreference(gravity);
}

size_t GraphNavNode::getHash() {
	return hash;
}

char GraphNavNode::getVoxel() {
	return voxel;
}

bool GraphNavNode::isWalkable() {
	return true;
}

int GraphNavNode::getAmountEdges() {
	boost::shared_lock<boost::shared_mutex> lock(EDGES_MUTEX);
	return edges.size();
}