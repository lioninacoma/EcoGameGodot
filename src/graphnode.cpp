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
	for (auto edge : edges)
		edge.second.reset();
	edges.clear();
}

void GraphNavNode::_init() {

}

void GraphNavNode::addEdge(std::shared_ptr<GraphEdge> edge) {
	boost::unique_lock<boost::shared_mutex> lock(EDGES_MUTEX);
	size_t nHash = (edge->getA()->getHash() != hash) ? edge->getA()->getHash() : edge->getB()->getHash();
	auto it = edges.find(nHash);
	if (it != edges.end()) return;
	edges[nHash] = edge;
}

void GraphNavNode::removeEdgeWithNode(size_t nHash) {
	boost::unique_lock<boost::shared_mutex> lock(EDGES_MUTEX);
	auto it = edges.find(nHash);
	if (it == edges.end()) return;
	edges.erase(nHash);
}

void GraphNavNode::setPoint(Vector3 point) {
	GraphNavNode::point = std::shared_ptr<Vector3>(new Vector3(point));
	GraphNavNode::hash = fn::hash(point);
}

void GraphNavNode::setVoxel(char voxel) {
	GraphNavNode::voxel = voxel;
}

void GraphNavNode::determineGravity(Vector3 cog) {
	Vector3 g = cog - *point;
	g.normalize();
	g *= 9.8;
	gravity = std::shared_ptr<Vector3>(new Vector3(g));
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
	return (edge->getA()->getHash() != hash) ? edge->getA() : edge->getB();
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