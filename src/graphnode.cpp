#include "graphnode.h"
#include "graphedge.h"

using namespace godot;

void GraphNavNode::_register_methods() {
	register_method("getPoint", &GraphNavNode::getPointU);
	register_method("getVoxel", &GraphNavNode::getVoxel);
	register_method("getGravity", &GraphNavNode::getGravityU);
}

Vector3 GraphNavNode::getDirectionVector(DIRECTION d) {
	switch (d) {
	case TOP:
		return Vector3(0, 1, 0);
	case BOTTOM:
		return Vector3(0, -1, 0);
	case WEST:
		return Vector3(-1, 0, 0);
	case EAST:
		return Vector3(1, 0, 0);
	case NORTH:
		return Vector3(0, 0, 1);
	case SOUTH:
		return Vector3(0, 0, -1);
	}
}

GraphNavNode::GraphNavNode(Vector3 point, char voxel) {
	GraphNavNode::point = std::shared_ptr<Vector3>(new Vector3(point));
	GraphNavNode::gravity = std::shared_ptr<Vector3>(new Vector3(0, -9.8, 0));
	GraphNavNode::hash = fn::hash(point);
	GraphNavNode::voxel = voxel;
	GraphNavNode::directionMask = 0;
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

void GraphNavNode::setDirection(DIRECTION d, bool set) {
	boost::unique_lock<boost::mutex> lock(DIRECTION_MASK_MUTEX);
	if (set) directionMask |= 1UL << d;
	else directionMask &= ~(1UL << d);
}

void GraphNavNode::setDirections(unsigned char mask) {
	boost::unique_lock<boost::mutex> lock(DIRECTION_MASK_MUTEX);
	directionMask |= mask;
}

void GraphNavNode::clearDirections() {
	boost::unique_lock<boost::mutex> lock(DIRECTION_MASK_MUTEX);
	directionMask &= 0;
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

int GraphNavNode::getAmountDirections() {
	boost::unique_lock<boost::mutex> lock(DIRECTION_MASK_MUTEX);
	int count = 0;
	unsigned char n = directionMask;
	while (n) {
		count += n & 1;
		n >>= 1;
	}
	return count;
}

vector<int> GraphNavNode::getDirections() {
	boost::unique_lock<boost::mutex> lock(DIRECTION_MASK_MUTEX);
	vector<int> directions;
	unsigned char mask = directionMask;
	int d = -1;
	while (mask && ++d < 6) {
		if (mask & 1) {
			directions.push_back(d);
		}
		mask >>= 1;
	}
	return directions;
}

PoolVector3Array GraphNavNode::getDirectionVectors() {
	PoolVector3Array directions;
	for (int d : getDirections()) {
		directions.push_back(getDirectionVector(static_cast<DIRECTION>(d)));
	}
	return directions;
}

bool GraphNavNode::isDirectionSet(DIRECTION d) {
	boost::unique_lock<boost::mutex> lock(DIRECTION_MASK_MUTEX);
	return (bool)((directionMask >> d) & 1U);
}

bool GraphNavNode::isWalkable() {
	PoolVector3Array directions = getDirectionVectors();
	Vector3 direction, gravity = getGravityU().normalized();
	int i;
	bool walkable = false;
	float deltaRad;
	for (i = 0; i < directions.size(); i++) {
		direction = directions[i].normalized();
		direction *= -1;
		deltaRad = abs(1.0 - direction.dot(gravity));
		walkable = deltaRad < .5;
		if (walkable) break;
	}
	return walkable;
}