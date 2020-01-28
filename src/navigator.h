#ifndef NAVIGATOR_H
#define NAVIGATOR_H

#include <Godot.hpp>
#include <Vector3.hpp>
#include <Mesh.hpp>
#include <Texture.hpp>
#include <ImmediateGeometry.hpp>

#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_types.hpp>

#include <deque>
#include <queue>
#include <unordered_set>
#include <unordered_map>
#include <limits>

#include "ecogame.h"
#include "constants.h"
#include "fn.h"
#include "chunk.h"
#include "graphnode.h"
#include "graphedge.h"

using namespace std;

namespace godot {

	class Navigator {

	private:
		template<typename T, typename priority_t>
		struct PriorityQueue {
			typedef std::pair<priority_t, T> PQElement;
			std::priority_queue<PQElement, std::vector<PQElement>,
				std::greater<PQElement>> elements;

			inline bool empty() const {
				return elements.empty();
			}

			inline void put(T item, priority_t priority) {
				elements.emplace(priority, item);
			}

			T get() {
				T best_item = elements.top().second;
				elements.pop();
				return best_item;
			}
		};

		unordered_map<size_t, GraphNode*>* nodes;
		boost::shared_timed_mutex NAV_NODES_MUTEX;
	public:
		static Navigator* get() {
			static Navigator* navigator = new Navigator();
			return navigator;
		};

		Navigator() {
			nodes = new unordered_map<size_t, GraphNode*>();
			//areas = new unordered_map<int, unordered_map<size_t, GraphNode*>*>();
			//areaVoxels = new unordered_map<int, unordered_set<char>*>();
		};
		~Navigator() {};

		void addEdge(GraphNode* a, GraphNode* b, float cost) {
			// ~600 MB
			auto edge = new GraphEdge(a, b, cost);
			a->addEdge(edge);
			b->addEdge(edge);
		}

		void addNode(GraphNode* node) {
			boost::unique_lock<boost::shared_timed_mutex> lock(NAV_NODES_MUTEX);
			nodes->emplace(node->getHash(), node);
		}

		void removeNode(GraphNode* node) {
			boost::unique_lock<boost::shared_timed_mutex> lock(NAV_NODES_MUTEX);
			size_t hash = node->getHash();
			GraphNode* neighbour;

			for (auto& next : node->getEdges()) {
				neighbour = (next.second->getA()->getHash() != hash) ? next.second->getA() : next.second->getB();
				neighbour->removeEdgeWithNode(hash);
			}

			nodes->erase(hash);
			delete node;
		}

		GraphNode* getNode(size_t h) {
			boost::shared_lock<boost::shared_timed_mutex> lock(NAV_NODES_MUTEX);
			auto it = nodes->find(h);
			if (it == nodes->end()) return NULL;
			return it->second;
		}

		void updateGraph(Chunk* chunk, Node* game) {
			//Godot::print(String("updating graph at {0} ...").format(Array::make(chunk->getOffset())));
			int x, y, z, drawOffsetY = 1;
			float nx, ny, nz;
			Vector3 chunkOffset;
			GraphNode *current, *neighbour;

			unordered_map<size_t, GraphNode*> nodeCache;
			deque<size_t> queue, volume;
			unordered_set<size_t> inque, ready;
			//unordered_map<size_t, GraphNode*>* area = new unordered_map<size_t, GraphNode*>();
			size_t cHash, nHash;

			/*auto dots = ImmediateGeometry::_new();
			dots->begin(Mesh::PRIMITIVE_POINTS);
			dots->set_color(Color(1, 0, 1, 1));
			for (auto &current : *chunkNodes) {
				dots->add_vertex(current.second);
			}
			dots->end();
			game->call_deferred("draw_debug_dots", dots);*/

			chunk->lockNodes();
			auto chunkNodes = chunk->getNodes();
			auto nodeChanges = chunk->getNodeChanges();
			for (auto& point : *chunkNodes) {
				if (nodeChanges->find(point.first) != nodeChanges->end()) {
					addNode(point.second);
				}

				nodeCache.insert(point);
			}
			for (auto& change : *nodeChanges) {
				if (change.second) continue;
				current = getNode(change.first);
				if (!current) continue;
				removeNode(current);
			}
			chunk->unlockNodes();

			/*auto geo = ImmediateGeometry::_new();
			geo->begin(Mesh::PRIMITIVE_LINES);
			geo->set_color(Color(0, 1, 0, 1));*/

			for (auto& node : nodeCache) {
				volume.push_back(node.first);
			}

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
					//area->emplace(cHash, current);

					for (z = -1; z < 2; z++)
						for (x = -1; x < 2; x++)
							for (y = -1; y < 2; y++) {
								if (!x && !y && !z) continue;

								nx = current->getPoint().x + x;
								ny = current->getPoint().y + y;
								nz = current->getPoint().z + z;

								nHash = fn::hash(Vector3(nx, ny, nz));
								neighbour = nodeCache[nHash];

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
								else if (inque.find(nHash) == inque.end() && ready.find(nHash) == ready.end()) {
									queue.push_back(nHash);
									inque.insert(nHash);
								}

								addEdge(current, neighbour, euclidean(current->getPoint(), neighbour->getPoint()));
							}
				}
			}

			chunk->clearNodeChanges();
			chunk->setNavigatable();

			Godot::print(String("graph at {0} updated.").format(Array::make(chunk->getOffset())));
			/*geo->end();
			game->call_deferred("draw_debug", geo);*/
		}

		float manhattan(Vector3 a, Vector3 b) {
			return abs(a.x - b.x) + abs(a.y - b.y) + abs(a.z - b.z);
		}

		float euclidean(Vector3 a, Vector3 b) {
			return a.distance_to(b);
		}

		float w(float distanceToGoal, float maxDistance) {
			float x = 1.0 - (distanceToGoal / maxDistance); // [0, 1]
			float n = MAX_WEIGHT_ROOT;
			float w = n * x - n;
			return 1 + w * w;
		}

		float h(GraphNode* node, GraphNode* goal, float maxDistance) {
			float distanceToGoal = manhattan(node->getPoint(), goal->getPoint());
			return w(distanceToGoal, maxDistance) * distanceToGoal;
		}

		void navigateToClosestVoxel(Vector3 startV, int voxel, int actorInstanceId, Node* game, EcoGame* lib) {
			//boost::shared_lock<boost::shared_timed_mutex> lock(NODE_MUTEX);
			PoolVector3Array path;

			GraphNode* startNode = NULL;

			float minDistanceStart = numeric_limits<float>::max();
			float currDistanceStart;

			NAV_NODES_MUTEX.lock_shared();
			for (auto& current : *nodes) {
				currDistanceStart = current.second->getPoint().distance_to(startV);

				if (currDistanceStart < minDistanceStart) {
					minDistanceStart = currDistanceStart;
					startNode = current.second;
				}
			}
			NAV_NODES_MUTEX.unlock_shared();

			if (!startNode) {
				game->call_deferred("set_path_actor", path, actorInstanceId);
				return;
			}
			
			GraphNode* currentNode;
			GraphNode* neighbourNode;

			unordered_map<size_t, size_t> cameFrom;
			unordered_map<size_t, float> costSoFar;
			size_t cHash, nHash, sHash = startNode->getHash();
			int x, y, z;
			float maxDist = 64;
			bool reachable = false;

			PriorityQueue<size_t, float> frontier;
			frontier.put(sHash, 0);
			costSoFar[sHash] = 0;
			Vector3 cPos;
			Vector3 vPos;
			int range = 1;

			while (!frontier.empty()) {
				cHash = frontier.get();
				currentNode = getNode(cHash);

				if (!currentNode) continue;

				if (manhattan(currentNode->getPoint(), startNode->getPoint()) > maxDist) {
					game->call_deferred("set_path_actor", path, actorInstanceId);
					return;
				}

				cPos = currentNode->getPoint() + Vector3(0, 1, 0);
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

					Vector3 currentPoint = currentNode->getPoint();

					while (true) {
						path.insert(0, currentPoint);
						//geo->add_vertex(currentPoint + Vector3(0, 0.25, 0));

						if (cameFrom.find(cHash) == cameFrom.end()) break;

						cHash = cameFrom[cHash];
						currentNode = currentNode->getNeighbour(cHash);
						currentPoint = currentNode->getPoint();

						path.insert(0, currentPoint);
						//geo->add_vertex(currentPoint + Vector3(0, 0.25, 0));
					}

					/*geo->end();
					game->call_deferred("draw_debug", geo);*/

					game->call_deferred("set_path_actor", path, actorInstanceId);
					return;
				}

				currentNode->lockEdges();
				for (auto& next : currentNode->getEdges()) {
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
				}
				currentNode->unlockEdges();
			}

			game->call_deferred("set_path_actor", path, actorInstanceId);
		}

		void navigate(Vector3 startV, Vector3 goalV, int actorInstanceId, Node* game) {
			//boost::shared_lock<boost::shared_timed_mutex> lock(NODE_MUTEX);
			//Godot::print(String("find path from {0} to {1}").format(Array::make(startV, goalV)));
			PoolVector3Array path;

			GraphNode* startNode = NULL;
			GraphNode* goalNode = NULL;

			float minDistanceStart = numeric_limits<float>::max();
			float minDistanceGoal = numeric_limits<float>::max();
			float currDistanceStart;
			float currDistanceGoal;

			NAV_NODES_MUTEX.lock_shared();
			for (auto& current : *nodes) {
				currDistanceStart = current.second->getPoint().distance_to(startV);
				currDistanceGoal = current.second->getPoint().distance_to(goalV);

				if (currDistanceStart < minDistanceStart) {
					minDistanceStart = currDistanceStart;
					startNode = current.second;
				}

				if (currDistanceGoal < minDistanceGoal) {
					minDistanceGoal = currDistanceGoal;
					goalNode = current.second;
				}
			}
			NAV_NODES_MUTEX.unlock_shared();

			if (!startNode || !goalNode) {
				game->call_deferred("set_path_actor", path, actorInstanceId);
				return;
			}

			GraphNode* currentNode;
			GraphNode* neighbourNode;

			unordered_map<size_t, size_t> cameFrom;
			unordered_map<size_t, float> costSoFar;
			size_t cHash, nHash, sHash = startNode->getHash(), gHash = goalNode->getHash();
			float maxDistance = manhattan(startNode->getPoint(), goalNode->getPoint());

			PriorityQueue<size_t, float> frontier;
			frontier.put(sHash, 0);
			costSoFar[sHash] = 0;

			while (!frontier.empty()) {
				cHash = frontier.get();
				currentNode = getNode(cHash);

				if (!currentNode) continue;

				if (cHash == gHash) {
					//Godot::print("path found");

					/*auto geo = ImmediateGeometry::_new();
					geo->begin(Mesh::PRIMITIVE_LINES);
					geo->set_color(Color(1, 0, 0, 1));*/

					Vector3 currentPoint = currentNode->getPoint();

					while (true) {
						path.insert(0, currentPoint);
						//geo->add_vertex(currentPoint + Vector3(0, 0.25, 0));

						if (cameFrom.find(cHash) == cameFrom.end()) break;

						cHash = cameFrom[cHash];
						currentNode = currentNode->getNeighbour(cHash);
						currentPoint = currentNode->getPoint();

						path.insert(0, currentPoint);
						//geo->add_vertex(currentPoint + Vector3(0, 0.25, 0));
					}

					/*geo->end();
					game->call_deferred("draw_debug", geo);*/

					game->call_deferred("set_path_actor", path, actorInstanceId);
					return;
				}

				currentNode->lockEdges();
				for (auto& next : currentNode->getEdges()) {
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
				}
				currentNode->unlockEdges();
			}

			game->call_deferred("set_path_actor", path, actorInstanceId);
		}
	};

}

#endif