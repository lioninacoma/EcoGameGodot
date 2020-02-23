#include "navigator.h"
#include "ecogame.h"
#include "chunk.h"
#include "graphedge.h"
#include "graphnode.h"

#include <boost/exception/diagnostic_information.hpp> 
#include <boost/exception_ptr.hpp> 

using namespace godot;

Navigator::Navigator() {
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

void Navigator::addNode(std::shared_ptr<GraphNavNode> node, Chunk* chunk) {
	try {
		int x, y, z;
		float nx, ny, nz;
		Vector3 chunkOffset;
		std::shared_ptr<Vector3> point;
		std::shared_ptr<GraphNavNode> neighbour;
		size_t nHash;

		addNode(node);

		for (z = -1; z < 2; z++)
			for (x = -1; x < 2; x++)
				for (y = -1; y < 2; y++) {
					if (!x && !y && !z) continue;

					point = node->getPoint();
					nx = point->x + x;
					ny = point->y + y;
					nz = point->z + z;

					nHash = fn::hash(Vector3(nx, ny, nz));
					neighbour = chunk->getNode(nHash);

					if (!neighbour) {
						chunkOffset.x = nx;
						chunkOffset.y = ny;
						chunkOffset.z = nz;
						chunkOffset = fn::toChunkCoords(chunkOffset);
						chunkOffset *= Vector3(CHUNK_SIZE_X, 0, CHUNK_SIZE_Z);

						if (chunk->getOffset() == chunkOffset) {
							continue;
						}

						neighbour = getNode(nHash);

						if (!neighbour) {
							continue;
						}
					}

					addEdge(node, neighbour, euclidean(fn::unreference(node->getPoint()), fn::unreference(neighbour->getPoint())));
				}
	}
	catch (const std::exception & e) {
		std::cerr << boost::diagnostic_information(e);
	}
}

void Navigator::updateGraph(std::shared_ptr<Chunk> chunk, Node* game) {
	//Godot::print(String("updating graph at {0} ...").format(Array::make(chunk->getOffset())));
	int x, y, z, drawOffsetY = 1;
	float nx, ny, nz;
	Vector3 chunkOffset;
	std::shared_ptr<Vector3> point;
	std::shared_ptr<GraphNavNode> current, neighbour;

	unordered_map<size_t, std::shared_ptr<GraphNavNode>> nodeCache;
	deque<size_t> queue, volume;
	unordered_set<size_t> inque, ready;
	size_t cHash, nHash;


	/*auto geo = ImmediateGeometry::_new();
	geo->begin(Mesh::PRIMITIVE_POINTS);
	geo->set_color(Color(1, 0, 0, 1));*/

	std::function<void(std::pair<size_t, std::shared_ptr<GraphNavNode>>)> nodeFn = [&](auto next) {
		addNode(next.second);
		nodeCache.insert(next);
		volume.push_back(next.first);
		Vector3 point = *next.second->getPoint();
		//geo->add_vertex(point);
	};
	chunk->forEachNode(nodeFn);

	/*geo->end();
	game->call_deferred("draw_debug_dots", geo);*/

	while (true) {
		while (!volume.empty()) {
			cHash = volume.front();
			volume.pop_front();
			if (ready.find(cHash) == ready.end()) {
				queue.push_back(cHash);
				inque.insert(cHash);
				break;
			}
		}
		if (queue.empty()) {
			break;
		}

		while (!queue.empty()) {
			cHash = queue.front();
			queue.pop_front();
			inque.erase(cHash);

			current = nodeCache[cHash];
			ready.insert(cHash);

			for (z = -1; z < 2; z++)
				for (x = -1; x < 2; x++)
					for (y = -1; y < 2; y++) {
						if (!x && !y && !z) continue;

						point = current->getPoint();
						nx = point->x + x;
						ny = point->y + y;
						nz = point->z + z;

						nHash = fn::hash(Vector3(nx, ny, nz));
						auto it = nodeCache.find(nHash);

						if (it == nodeCache.end()) {
							chunkOffset.x = nx;
							chunkOffset.y = ny;
							chunkOffset.z = nz;
							chunkOffset = fn::toChunkCoords(chunkOffset);
							chunkOffset *= Vector3(CHUNK_SIZE_X, 0, CHUNK_SIZE_Z);

							if (chunk->getOffset() == chunkOffset) {
								continue;
							}

							neighbour = getNode(nHash);

							if (!neighbour) {
								continue;
							}
						}
						else {
							neighbour = it->second;

							if (inque.find(nHash) == inque.end() && ready.find(nHash) == ready.end()) {
								queue.push_back(nHash);
								inque.insert(nHash);
							}
						}

						addEdge(current, neighbour, euclidean(fn::unreference(current->getPoint()), fn::unreference(neighbour->getPoint())));
					}
		}
	}

	chunk->setNavigatable();
	//Godot::print(String("graph at {0} updated.").format(Array::make(chunk->getOffset())));
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

void Navigator::setPathActor(Array path, int actorInstanceId, Node* game) {
	game->call_deferred("set_path_actor", path, actorInstanceId);
}

void Navigator::navigateToClosestVoxel(Vector3 startV, int voxel, int actorInstanceId, Node* game, EcoGame* lib) {
	//Godot::print(String("find path from {0} to closest voxel of type {1}").format(Array::make(startV, voxel)));
	Array path;
	std::shared_ptr<GraphNavNode> startNode = lib->getNode(startV);

	if (!startNode) {
		setPathActor(path, actorInstanceId, game);
		return;
	}

	std::shared_ptr<Vector3> currentPoint;
	std::shared_ptr<GraphNavNode> currentNode;
	std::shared_ptr<GraphNavNode> neighbourNode;

	float maxDist = 96;
	unordered_map<size_t, std::shared_ptr<GraphNavNode>> nodeCache;
	vector<std::shared_ptr<Chunk>> chunks = lib->getChunksInRange(fn::unreference(startNode->getPoint()), maxDist);
	//Godot::print(String("chunks {0}").format(Array::make(chunks.size())));

	for (auto chunk : chunks) {
		std::function<void(std::pair<size_t, std::shared_ptr<GraphNavNode>>)> nodeFn = [&](auto next) {
			nodeCache.insert(next);
		};
		chunk->forEachNode(nodeFn);
	}

	unordered_map<size_t, size_t> cameFrom;
	unordered_map<size_t, float> costSoFar;
	size_t cHash, nHash, sHash = startNode->getHash();
	int x, y, z;
	bool reachable = false;

	PriorityQueue<size_t, float> frontier;
	frontier.put(sHash, 0);
	costSoFar[sHash] = 0;
	Vector3 cPos;
	Vector3 vPos;
	int range = 1;

	while (!frontier.empty()) {
		cHash = frontier.get();
		auto nodeIt = nodeCache.find(cHash);

		if (nodeIt == nodeCache.end())
			currentNode = getNode(cHash);
		else currentNode = nodeIt->second;

		if (!currentNode) continue;
		
		currentPoint = currentNode->getPoint();

		if (manhattan(fn::unreference(currentPoint), fn::unreference(currentPoint)) > maxDist) {
			setPathActor(path, actorInstanceId, game);
			return;
		}

		cPos = fn::unreference(currentPoint) + Vector3(0, 1, 0);
		for (z = -range; z < range && !reachable; z++)
			for (y = -range; y < range && !reachable; y++)
				for (x = -range; x < range && !reachable; x++) {
					if (!x && !y && !z) continue;
					vPos.x = cPos.x + x;
					vPos.y = cPos.y + y;
					vPos.z = cPos.z + z;
					reachable = lib->getVoxel(vPos) == voxel;
				}

		if (reachable) {
			//Godot::print("path found");

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

			setPathActor(path, actorInstanceId, game);
			return;
		}

		std::function<void(std::pair<size_t, std::shared_ptr<GraphEdge>>)> fn = [&](auto next) {
			neighbourNode = (next.second->getA()->getHash() != cHash) ? next.second->getA() : next.second->getB();
			nHash = neighbourNode->getHash();
			float newCost = costSoFar[cHash] + next.second->getCost();
			if (costSoFar.find(nHash) == costSoFar.end()
				|| newCost < costSoFar[nHash]) {
				costSoFar[nHash] = newCost;
				float priority = newCost + 1.0;
				frontier.put(nHash, priority);
				cameFrom[nHash] = cHash;
			}
		};
		currentNode->forEachEdge(fn);
	}

	setPathActor(path, actorInstanceId, game);
}

void Navigator::navigate(Vector3 startV, Vector3 goalV, int actorInstanceId, Node* game, EcoGame* lib) {
	//Godot::print(String("find path from {0} to {1}").format(Array::make(startV, goalV)));
	bpt::ptime start, stop;
	bpt::time_duration dur;
	long ms = 0;

	start = bpt::microsec_clock::local_time();

	Array path;
	std::shared_ptr<GraphNavNode> startNode = lib->getNode(startV);
	std::shared_ptr<GraphNavNode> goalNode = lib->getNode(goalV);

	if (!startNode || !goalNode) {
		setPathActor(path, actorInstanceId, game);
		return;
	}

	std::shared_ptr<GraphNavNode> currentNode;
	std::shared_ptr<GraphNavNode> neighbourNode;

	unordered_map<size_t, std::shared_ptr<GraphNavNode>> nodeCache;
	vector<std::shared_ptr<Chunk>> chunks = lib->getChunksRay(fn::unreference(startNode->getPoint()), fn::unreference(goalNode->getPoint()));

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

			setPathActor(path, actorInstanceId, game);

			stop = bpt::microsec_clock::local_time();
			dur = stop - start;
			ms = dur.total_milliseconds();

			//Godot::print(String("path from {0} to {1} found in {2} ms").format(Array::make(startV, goalV, ms)));
			return;
		}

		std::function<void(std::pair<size_t, std::shared_ptr<GraphEdge>>)> fn = [&](auto next) {
			neighbourNode = (next.second->getA()->getHash() != cHash) ? next.second->getA() : next.second->getB();
			nHash = neighbourNode->getHash();
			float newCost = costSoFar[cHash] + next.second->getCost();
			if (costSoFar.find(nHash) == costSoFar.end()
				|| newCost < costSoFar[nHash]) {
				costSoFar[nHash] = newCost;
				float priority = newCost + h(neighbourNode, goalNode, maxDistance);
				frontier.put(nHash, priority);
				cameFrom[nHash] = cHash;
			}
		};
		currentNode->forEachEdge(fn);
	}

	setPathActor(path, actorInstanceId, game);
}