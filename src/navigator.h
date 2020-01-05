#ifndef NAVIGATOR_H
#define NAVIGATOR_H

#include <Godot.hpp>
#include <Reference.hpp>
#include <Vector3.hpp>
#include <ImmediateGeometry.hpp>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/graph/astar_search.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_types.hpp>

#include <deque>
#include <queue>
#include <unordered_set>
#include <unordered_map>
#include <limits>

#include "constants.h"
#include "fn.h"
#include "chunk.h"

using namespace std;

namespace godot {

	class Navigator : public Reference {
		GODOT_CLASS(Navigator, Reference)

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

		class GraphNode;

		class GraphEdge {
		private:
			GraphNode* a;
			GraphNode* b;
			float cost;
		public:
			GraphEdge(GraphNode* a, GraphNode* b, float cost) {
				GraphEdge::a = a;
				GraphEdge::b = b;
				GraphEdge::cost = cost;
			};
			GraphNode* getA() {
				return a;
			};
			GraphNode* getB() {
				return b;
			};
			float getCost() {
				return cost;
			};
		};

		class GraphNode {
		private:
			Vector3 point;
			unordered_map<size_t, GraphEdge*> edges;
			size_t hash;
		public:
			GraphNode(Vector3 point) {
				GraphNode::point = point;
				GraphNode::hash = fn::hash(point);
			};
			void addEdge(GraphEdge* edge) {
				size_t nHash = (edge->getA()->getHash() != hash) ? edge->getA()->getHash() : edge->getB()->getHash();
				edges[nHash] = edge;
			};
			void removeEdgeWithNode(size_t nHash) {
				auto it = edges.find(nHash);
				if (it == edges.end()) return;
				edges.erase(nHash);
			};
			GraphEdge* getEdgeWithNode(size_t nHash) {
				auto it = edges.find(nHash);
				if (it == edges.end()) return NULL;
				return it->second;
			};
			GraphNode* getNeighbour(size_t nHash) {
				GraphEdge* edge = getEdgeWithNode(nHash);
				if (!edge) return NULL;
				return (edge->getA()->getHash() != hash) ? edge->getA() : edge->getB();
			};
			unordered_map<size_t, GraphEdge*> getEdges() {
				return edges;
			};
			Vector3 getPoint() {
				return point;
			};
			size_t getHash() {
				return hash;
			};
			bool operator == (const GraphNode& o) const {
				return hash == o.hash;
			};
		};

		unordered_map<size_t, GraphNode*>* nodes;
		boost::mutex mutex;
	public:
		static void _register_methods() {
			register_method("updateGraph", &Navigator::updateGraph);
			register_method("navigate", &Navigator::navigate);
		}

		Navigator() {
			nodes = new unordered_map<size_t, GraphNode*>();
		};
		~Navigator() {};

		void _init() {
			
		}

		void addNode(GraphNode* n) {
			boost::unique_lock<boost::mutex> lock(mutex);
			nodes->insert(pair<size_t, GraphNode*>(n->getHash(), n));
		}

		void addNodes(unordered_map<size_t, GraphNode*> nodeMap) {
			boost::unique_lock<boost::mutex> lock(mutex);
			for (auto &n : nodeMap) {
				nodes->insert(pair<size_t, GraphNode*>(n.first, n.second));
			}
		}

		void addEdge(GraphNode* a, GraphNode* b, float cost) {
			boost::unique_lock<boost::mutex> lock(mutex);
			auto edge = new GraphEdge(a, b, cost);
			a->addEdge(edge);
			b->addEdge(edge);
		}

		void removeNode(GraphNode* node) {
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
			boost::unique_lock<boost::mutex> lock(mutex);
			auto it = nodes->find(h);
			if (it == nodes->end()) return NULL;
			return it->second;
		}

		void updateGraph(Chunk* chunk, Node* game) {
			int x, y, z, drawOffsetY = 1;
			float nx, ny, nz;
			Vector3 chunkOffset;
			GraphNode *current, *neighbour;
			auto chunkPoints = chunk->getNodes();
			auto nodeChanges = chunk->getNodeChanges();

			unordered_map<size_t, GraphNode*> nodeCache;

			/*auto dots = ImmediateGeometry::_new();
			dots->begin(Mesh::PRIMITIVE_POINTS);
			dots->set_color(Color(1, 0, 1, 1));
			for (auto &current : *chunkPoints) {
				dots->add_vertex(current.second);
			}
			dots->end();
			game->call_deferred("draw_debug_dots", dots);*/

			mutex.lock();
			for (auto& point : *chunkPoints) {
				if (nodeChanges->find(point.first) != nodeChanges->end()) {
					current = new GraphNode(point.second);
					nodes->insert(pair<size_t, GraphNode*>(point.first, current));;
				}
				else {
					auto it = nodes->find(point.first);
					if (it == nodes->end()) continue;
					current = it->second;
				}

				nodeCache[point.first] = current;
			}
			for (auto& change : *nodeChanges) {
				if (change.second) continue;
				auto it = nodes->find(change.first);
				if (it == nodes->end()) continue;
				current = it->second;
				removeNode(current);
			}
			mutex.unlock();

			/*auto geo = ImmediateGeometry::_new();
			geo->begin(Mesh::PRIMITIVE_LINES);
			geo->set_color(Color(0, 1, 0, 1));*/

			for (auto& change : *nodeChanges) {
				if (!change.second) continue;
				current = nodeCache[change.first];

				for (z = -1; z < 2; z++)
					for (x = -1; x < 2; x++)
						for (y = -1; y < 2; y++) {
							if (!x && !y && !z) continue;

							nx = current->getPoint().x + x;
							ny = current->getPoint().y + y;
							nz = current->getPoint().z + z;

							size_t nHash = fn::hash(Vector3(nx, ny, nz));
							auto it = chunkPoints->find(nHash);

							if (it == chunkPoints->end()) {
								chunkOffset.x = nx;
								chunkOffset.y = ny;
								chunkOffset.z = nz;
								chunkOffset = fn::toChunkCoords(chunkOffset);
								chunkOffset *= Vector3(CHUNK_SIZE_X, 0, CHUNK_SIZE_Z);
								if (chunk->getOffset() == chunkOffset) continue;

								// node from another chunk
								neighbour = getNode(nHash);
								if (!neighbour) continue;
							}
							else {
								neighbour = nodeCache[nHash];
							}

							addEdge(current, neighbour, 1.0);
						}
			}

			nodeChanges->clear();

			Godot::print(String("graph at {0} updated").format(Array::make(chunk->getOffset())));
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

		PoolVector3Array navigate(Vector3 startV, Vector3 goalV, Node* game) {
			boost::unique_lock<boost::mutex> lock(mutex);
			PoolVector3Array path;

			GraphNode* startNode = NULL;
			GraphNode* goalNode = NULL;

			float minDistanceStart = numeric_limits<float>::max();
			float minDistanceGoal = numeric_limits<float>::max();
			float currDistanceStart;
			float currDistanceGoal;

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

			if (!startNode || !goalNode) return path;

			GraphNode* currentNode;
			GraphNode* neighbourNode;

			Godot::print(String("find path from {0} to {1}").format(Array::make(startNode->getPoint(), goalNode->getPoint())));

			unordered_map<size_t, size_t> cameFrom;
			unordered_map<size_t, float> costSoFar;
			size_t cHash, nHash, sHash = startNode->getHash(), gHash = goalNode->getHash();
			float maxDistance = manhattan(startNode->getPoint(), goalNode->getPoint());

			PriorityQueue<size_t, float> frontier;
			frontier.put(sHash, 0);
			costSoFar[sHash] = 0;

			while (!frontier.empty()) {
				cHash = frontier.get();
				currentNode = nodes->at(cHash);

				if (!currentNode) continue;

				if (cHash == gHash) {
					Godot::print("path found");

					auto geo = ImmediateGeometry::_new();
					geo->begin(Mesh::PRIMITIVE_LINES);
					geo->set_color(Color(1, 0, 0, 1));

					Vector3 currentPoint = currentNode->getPoint();

					while (true) {
						path.append(currentPoint);
						geo->add_vertex(currentPoint + Vector3(0, 0.25, 0));

						if (cameFrom.find(cHash) == cameFrom.end()) break;

						cHash = cameFrom[cHash];
						currentNode = currentNode->getNeighbour(cHash);
						currentPoint = currentNode->getPoint();

						path.append(currentPoint);
						geo->add_vertex(currentPoint + Vector3(0, 0.25, 0));
					}

					geo->end();
					game->call_deferred("draw_debug", geo);

					return path;
				}

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
			}

			return path;
		}
	};

}

#endif