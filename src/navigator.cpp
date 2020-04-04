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
}

Navigator::~Navigator() {

}

float Navigator::w(float distanceToGoal, float maxDistance) {
	float x = 1.0 - (distanceToGoal / maxDistance); // [0, 1]
	float n = MAX_WEIGHT_ROOT;
	float w = n * x - n;
	return 1 + w * w;
}

float Navigator::h(std::shared_ptr<GraphNavNode> node, std::shared_ptr<GraphNavNode> goal, float maxDistance) {
	float distanceToGoal = fn::manhattan(node->getPointU(), goal->getPointU());
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
	vector<std::shared_ptr<Chunk>> chunks = world->getChunksRay(startNode->getPointU(), goalNode->getPointU());

	for (auto chunk : chunks) {
		std::function<void(std::pair<size_t, std::shared_ptr<GraphNavNode>>)> nodeFn = [&](auto next) {
			nodeCache.insert(next);
		};
		chunk->forEachNode(nodeFn);
	}

	PriorityQueue<size_t, float> frontier;
	unordered_map<size_t, Vector3> frontierPoints;
	unordered_map<size_t, size_t> cameFrom;
	unordered_map<size_t, float> costSoFar;
	
	size_t cHash, nHash, sHash = startNode->getHash(), gHash = goalNode->getHash();
	float maxDistance = fn::manhattan(startNode->getPointU(), goalNode->getPointU());

	frontier.put(sHash, 0);
	costSoFar[sHash] = 0;

	while (!frontier.empty()) {
		cHash = frontier.get();
		auto nodeIt = nodeCache.find(cHash);

		if (nodeIt == nodeCache.end()) {
			currentNode = world->getNode(frontierPoints[cHash]);
		}
		else {
			currentNode = nodeIt->second;
		}

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
				if (!currentNode) currentNode = world->getNode(frontierPoints[cHash]);
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
					frontierPoints[nHash] = neighbourNode->getPointU();
					cameFrom[nHash] = cHash;
				}
			}
		};
		currentNode->forEachEdge(fn);
	}

	setPathActor(path, actorInstanceId);
}