#include "navigator.h"
#include "voxelworld.h"
#include "ecogame.h"
#include "chunk.h"
#include "graphedge.h"
#include "graphnode.h"

#include <boost/exception/diagnostic_information.hpp> 
#include <boost/exception_ptr.hpp> 

using namespace godot;

Navigator::Navigator(std::shared_ptr<VoxelWorld> world) {
	Navigator::world = world;
	nodes = new unordered_map<size_t, std::shared_ptr<GraphNavNode>>();
}

Navigator::~Navigator() {
	delete nodes;
}

void Navigator::addEdge(std::shared_ptr<GraphNavNode> a, std::shared_ptr<GraphNavNode> b, float cost) {
	// ~600 MB
	auto edge = std::shared_ptr<GraphEdge>(new GraphEdge(a, b, cost));
	a->addEdge(edge);
	b->addEdge(edge);
}

void Navigator::addNode(std::shared_ptr<GraphNavNode> node) {
	boost::unique_lock<boost::shared_mutex> lock(NAV_NODES_MUTEX);
	auto it = nodes->find(node->getHash());
	if (it != nodes->end()) return;
	nodes->emplace(node->getHash(), node);
}

void Navigator::removeNode(std::shared_ptr<GraphNavNode> node) {
	try {
		size_t hash = node->getHash();
		std::shared_ptr<GraphNavNode> neighbour;

		std::function<void(std::pair<size_t, std::shared_ptr<GraphEdge>>)> lambda = [&](auto next) {
			neighbour = (next.second->getA()->getHash() != hash) ? next.second->getA() : next.second->getB();
			neighbour->removeEdgeWithNode(hash);
		};
		node->forEachEdge(lambda);

		NAV_NODES_MUTEX.lock();
		nodes->erase(hash);
		NAV_NODES_MUTEX.unlock();

		//node.reset();
	}
	catch (const std::exception & e) {
		std::cerr << boost::diagnostic_information(e);
	}
}

std::shared_ptr<GraphNavNode> Navigator::getNode(size_t h) {
	boost::shared_lock<boost::shared_mutex> lock(NAV_NODES_MUTEX);
	auto it = nodes->find(h);
	if (it == nodes->end()) return NULL;
	return it->second;
}

std::shared_ptr<GraphNavNode> Navigator::fetchOrCreateNode(Vector3 position, Chunk* chunk) {
	std::shared_ptr<GraphNavNode> node;
	size_t hash = fn::hash(position);
	Vector3 chunkOffset = position;
	chunkOffset = fn::toChunkCoords(chunkOffset);
	chunkOffset *= Vector3(CHUNK_SIZE_X, 0, CHUNK_SIZE_Z);

	if (chunk->getOffset() != chunkOffset) {
		auto neighbourChunk = world->getChunk(position);
		if (neighbourChunk != NULL) {
			node = neighbourChunk->getNode(hash);
		}
	}
	else {
		node = chunk->getNode(hash);
	}

	if (!node) {
		node = std::shared_ptr<GraphNavNode>(GraphNavNode::_new());
		node->setPoint(position);
		node->setVoxel(1);
		chunk->addNode(node);
		//addNode(node);
	}

	return node;
}

const bool SHOW_NODES_DEBUG = false;
void Navigator::addFaceNodes(Vector3 a, Vector3 b, Vector3 c, Chunk* chunk) {
	auto lib = EcoGame::get();
	Node* game = lib->getNode();

	std::shared_ptr<GraphNavNode> aNode, bNode, cNode;
	aNode = fetchOrCreateNode(a, chunk);
	bNode = fetchOrCreateNode(b, chunk);
	cNode = fetchOrCreateNode(c, chunk);
	
	addEdge(aNode, bNode, euclidean(aNode->getPointU(), bNode->getPointU()));
	addEdge(aNode, cNode, euclidean(aNode->getPointU(), cNode->getPointU()));
	addEdge(bNode, cNode, euclidean(bNode->getPointU(), cNode->getPointU()));

	if (SHOW_NODES_DEBUG) {
		ImmediateGeometry* geo;

		geo = ImmediateGeometry::_new();
		geo->begin(Mesh::PRIMITIVE_POINTS);
		geo->set_color(Color(1, 0, 0, 1));
		geo->add_vertex(aNode->getPointU());
		geo->add_vertex(bNode->getPointU());
		geo->add_vertex(cNode->getPointU());
		geo->end();
		game->call_deferred("draw_debug_dots", geo);

		geo = ImmediateGeometry::_new();
		geo->begin(Mesh::PRIMITIVE_LINES);
		geo->set_color(Color(0, 1, 0, 1));
		geo->add_vertex(aNode->getPointU());
		geo->add_vertex(bNode->getPointU());
		geo->add_vertex(bNode->getPointU());
		geo->add_vertex(cNode->getPointU());
		geo->add_vertex(cNode->getPointU());
		geo->add_vertex(aNode->getPointU());
		geo->end();
		game->call_deferred("draw_debug", geo);
	}
}

float Navigator::manhattan(Vector3 a, Vector3 b) {
	return abs(a.x - b.x) + abs(a.y - b.y) + abs(a.z - b.z);
}

float Navigator::euclidean(Vector3 a, Vector3 b) {
	return a.distance_to(b);
}

float Navigator::w(float distanceToGoal, float maxDistance) {
	float x = 1.0 - (distanceToGoal / maxDistance); // [0, 1]
	float n = MAX_WEIGHT_ROOT;
	float w = n * x - n;
	return 1 + w * w;
}

float Navigator::h(std::shared_ptr<GraphNavNode> node, std::shared_ptr<GraphNavNode> goal, float maxDistance) {
	float distanceToGoal = manhattan(fn::unreference(node->getPoint()), fn::unreference(goal->getPoint()));
	return w(distanceToGoal, maxDistance) * distanceToGoal;
}

void Navigator::setPathActor(Array path, int actorInstanceId) {
	Node* game = EcoGame::get()->getNode();
	game->call_deferred("set_path_actor", path, actorInstanceId);
}

void Navigator::navigate(Vector3 startV, Vector3 goalV, int actorInstanceId) {
	//Godot::print(String("find path from {0} to {1}").format(Array::make(startV, goalV)));
	bpt::ptime start, stop;
	bpt::time_duration dur;
	long ms = 0;

	start = bpt::microsec_clock::local_time();

	Array path;
	std::shared_ptr<GraphNavNode> startNode = world->getNode(startV);
	std::shared_ptr<GraphNavNode> goalNode = world->getNode(goalV);

	if (!startNode || !goalNode) {
		setPathActor(path, actorInstanceId);
		return;
	}

	std::shared_ptr<GraphNavNode> currentNode;
	std::shared_ptr<GraphNavNode> neighbourNode;

	unordered_map<size_t, std::shared_ptr<GraphNavNode>> nodeCache;
	vector<std::shared_ptr<Chunk>> chunks = world->getChunksRay(fn::unreference(startNode->getPoint()), fn::unreference(goalNode->getPoint()));

	for (auto chunk : chunks) {
		std::function<void(std::pair<size_t, std::shared_ptr<GraphNavNode>>)> nodeFn = [&](auto next) {
			nodeCache.insert(next);
		};
		chunk->forEachNode(nodeFn);
	}

	unordered_map<size_t, size_t> cameFrom;
	unordered_map<size_t, float> costSoFar;
	size_t cHash, nHash, sHash = startNode->getHash(), gHash = goalNode->getHash();
	float maxDistance = manhattan(fn::unreference(startNode->getPoint()), fn::unreference(goalNode->getPoint()));

	PriorityQueue<size_t, float> frontier;
	frontier.put(sHash, 0);
	costSoFar[sHash] = 0;

	while (!frontier.empty()) {
		cHash = frontier.get();
		auto nodeIt = nodeCache.find(cHash);

		if (nodeIt == nodeCache.end())
			currentNode = getNode(cHash);
		else currentNode = nodeIt->second;

		if (!currentNode) continue;

		if (cHash == gHash) {
			/*auto geo = ImmediateGeometry::_new();
			geo->begin(Mesh::PRIMITIVE_LINES);
			geo->set_color(Color(1, 0, 0, 1));*/

			while (true) {
				path.push_front(currentNode.get());
				//geo->add_vertex(currentPoint + Vector3(0, 0.25, 0));

				if (cameFrom.find(cHash) == cameFrom.end()) break;

				cHash = cameFrom[cHash];
				currentNode = currentNode->getNeighbour(cHash);
				if (!currentNode) currentNode = getNode(cHash);
				if (!currentNode) return;

				path.push_front(currentNode.get());
				//geo->add_vertex(currentPoint + Vector3(0, 0.25, 0));
			}

			/*geo->end();
			game->call_deferred("draw_debug", geo);*/

			setPathActor(path, actorInstanceId);

			stop = bpt::microsec_clock::local_time();
			dur = stop - start;
			ms = dur.total_milliseconds();

			//Godot::print(String("path from {0} to {1} found in {2} ms").format(Array::make(startV, goalV, ms)));
			return;
		}

		std::function<void(std::pair<size_t, std::shared_ptr<GraphEdge>>)> fn = [&](auto next) {
			neighbourNode = (next.second->getA()->getHash() != cHash) ? next.second->getA() : next.second->getB();
			if (neighbourNode->isWalkable()) {
				nHash = neighbourNode->getHash();
				float newCost = costSoFar[cHash] + next.second->getCost();
				if (costSoFar.find(nHash) == costSoFar.end()
					|| newCost < costSoFar[nHash]) {
					costSoFar[nHash] = newCost;
					float priority = newCost + h(neighbourNode, goalNode, maxDistance);
					frontier.put(nHash, priority);
					cameFrom[nHash] = cHash;
				}
			}
		};
		currentNode->forEachEdge(fn);
	}

	setPathActor(path, actorInstanceId);
}