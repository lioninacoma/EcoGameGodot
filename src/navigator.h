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
			vector<GraphEdge*>* edges;
			size_t hash;
		public:
			GraphNode(Vector3 point) {
				GraphNode::point = point;
				GraphNode::hash = fn::hash(point);
				edges = new vector<GraphEdge*>();
			};
			void addEdge(GraphEdge* edge) {
				edges->push_back(edge);
			};
			vector<GraphEdge*>* getEdges() {
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

		void addEdge(GraphNode* a, GraphNode* b, float cost) {
			boost::unique_lock<boost::mutex> lock(mutex);
			auto edge = new GraphEdge(a, b, cost);
			a->addEdge(edge);
			b->addEdge(edge);
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
			GraphNode *current, *neighbour;
			auto chunkPoints = chunk->getNodes();

			deque<size_t> queue;
			unordered_set<size_t> queueHashes;
			unordered_set<size_t> ready;

			/*auto dots = ImmediateGeometry::_new();
			dots->begin(Mesh::PRIMITIVE_POINTS);
			dots->set_color(Color(1, 0, 1, 1));

			for (auto &current : *chunkPoints) {
				dots->add_vertex(current.second);
			}

			dots->end();
			game->call_deferred("draw_debug_dots", dots);*/

			/*auto geo = ImmediateGeometry::_new();
			geo->begin(Mesh::PRIMITIVE_LINES);
			geo->set_color(Color(0, 1, 0, 1));*/

			while (ready.size() != chunkPoints->size()) {
				//Godot::print(String("Chunk{0}: {1}/{2}").format(Array::make(chunk->getOffset(), ready.size(), chunkPoints->size())));

				for (auto &current : *chunkPoints) {
					if (ready.find(current.first) == ready.end()) {
						queue.push_back(current.first);
						queueHashes.insert(current.first);
						addNode(new GraphNode(current.second));
						//Godot::print(String("add: {0}").format(Array::make(current.first)));
						break;
					}
				}

				while (!queue.empty()) {
					size_t cHash = queue.front();
					queue.pop_front();
					queueHashes.erase(cHash);
					current = getNode(cHash);
					ready.insert(cHash);

					//Godot::print(String("find: {0} = {1}").format(Array::make(cHash, cIndex)));
					//Godot::print(String("current: {0}").format(Array::make(Vector3(current.x, current.y, current.z))));
					
					if (!current) continue;

					for (z = -1; z < 2; z++)
						for (x = -1; x < 2; x++)
							for (y = -1; y < 2; y++) {
								if (!x && !y && !z) continue;

								nx = current->getPoint().x + x;
								ny = current->getPoint().y + y;
								nz = current->getPoint().z + z;

								size_t nHash = fn::hash(Vector3(nx, ny, nz));
								auto it = chunkPoints->find(nHash);

								//if (it == chunkPoints->end()) continue;
								if (it == chunkPoints->end()) {
									// pre existing neighbour node
									neighbour = getNode(nHash);
									if (!neighbour) continue;
								}
								else {
									if (ready.find(nHash) == ready.end() && queueHashes.find(nHash) == queueHashes.end()) {
										queue.push_back(nHash);
										queueHashes.insert(nHash);
									}

									// neighbour node in new mesh
									neighbour = new GraphNode(it->second);
									addNode(neighbour);
								}

								/*geo->add_vertex(*current->getPoint());
								geo->add_vertex(*neighbour->getPoint());*/

								const float cost = manhattan(current->getPoint(), neighbour->getPoint());
								//addEdge(current, neighbour, cost);
								addEdge(current, neighbour, 1.0);
							}
				}
			}

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

			//Godot::print(String("startP: {0}, goalP: {1}").format(Array::make(Vector3(startP.x, startP.y, startP.z), Vector3(goalP.x, goalP.y, goalP.z))));

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
						currentPoint = nodes->at(cHash)->getPoint();

						path.append(currentPoint);
						geo->add_vertex(currentPoint + Vector3(0, 0.25, 0));
					}

					geo->end();
					game->call_deferred("draw_debug", geo);

					return path;
				}

				for (auto& next : *currentNode->getEdges()) {
					neighbourNode = (next->getA()->getHash() != cHash) ? next->getA() : next->getB();
					nHash = neighbourNode->getHash();
					float newCost = costSoFar[cHash] + next->getCost();
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