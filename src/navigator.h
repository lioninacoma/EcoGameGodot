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

#include "constants.h"
#include "fn.h"
#include "chunk.h"

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
			Vector3 point, offset;
			unordered_map<size_t, GraphEdge*> edges;
			size_t hash;
			int type;
			int voxel;
		public:
			GraphNode(Vector3 point, Vector3 offset, char voxel) {
				GraphNode::point = point;
				GraphNode::offset = offset;
				GraphNode::hash = fn::hash(point);
				GraphNode::type = 0;
				GraphNode::voxel = voxel;
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
			void setType(int type) {
				GraphNode::type = type;
			}
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
			Vector3 getOffset() {
				return offset;
			}
			size_t getHash() {
				return hash;
			};
			int getType() {
				return type;
			};
			int getVoxel() {
				return voxel;
			};
			bool operator == (const GraphNode& o) const {
				return hash == o.hash;
			};
		};

		unordered_map<size_t, GraphNode*>* nodes;
		unordered_map<int, unordered_map<size_t, GraphNode*>*>* areas;
		unordered_map<int, unordered_set<int>*>* areaVoxels;
		boost::mutex mutex;
		int currentType = 1;
	public:
		static Navigator* get() {
			static Navigator* navigator = new Navigator();
			return navigator;
		};

		Navigator() {
			nodes = new unordered_map<size_t, GraphNode*>();
			areas = new unordered_map<int, unordered_map<size_t, GraphNode*>*>();
			areaVoxels = new unordered_map<int, unordered_set<int>*>();
		};
		~Navigator() {};

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

		int getNextType() {
			int type = currentType;
			currentType++;
			return type;
		}

		void updateGraph(Chunk* chunk, Node* game) {
			int x, y, z, drawOffsetY = 1;
			float nx, ny, nz;
			Vector3 chunkOffset;
			GraphNode *current, *neighbour;
			auto chunkPoints = chunk->getNodes();
			auto nodeChanges = chunk->getNodeChanges();

			unordered_map<size_t, GraphNode*> nodeCache;
			deque<size_t> queue, volume;
			unordered_set<size_t> inque, ready;
			unordered_set<int> neighbourTypes;
			unordered_map<size_t, GraphNode*>* area = new unordered_map<size_t, GraphNode*>();
			size_t cHash, nHash;
			int type;

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
					current = new GraphNode(point.second.getPosition(), chunk->getOffset(), point.second.getType());
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
				areas->at(current->getType())->erase(change.first);
				removeNode(current);
			}
			mutex.unlock();

			/*auto geo = ImmediateGeometry::_new();
			geo->begin(Mesh::PRIMITIVE_LINES);
			geo->set_color(Color(0, 1, 0, 1));*/

			for (auto& change : *nodeChanges) {
				if (!change.second) continue;
				volume.push_back(change.first);
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

				neighbourTypes.clear();

				while (!queue.empty()) {
					cHash = queue.front();
					queue.pop_front();
					inque.erase(cHash);

					current = nodeCache[cHash];
					ready.insert(cHash);
					area->emplace(cHash, current);

					for (z = -1; z < 2; z++)
						for (x = -1; x < 2; x++)
							for (y = -1; y < 2; y++) {
								if (!x && !y && !z) continue;

								nx = current->getPoint().x + x;
								ny = current->getPoint().y + y;
								nz = current->getPoint().z + z;

								nHash = fn::hash(Vector3(nx, ny, nz));

								if (chunkPoints->find(nHash) == chunkPoints->end()) {
									chunkOffset.x = nx;
									chunkOffset.y = ny;
									chunkOffset.z = nz;
									chunkOffset = fn::toChunkCoords(chunkOffset);
									chunkOffset *= Vector3(CHUNK_SIZE_X, 0, CHUNK_SIZE_Z);
									if (chunk->getOffset() == chunkOffset) continue;
									neighbour = getNode(nHash);
									if (!neighbour) continue;
								
									mutex.lock();
									int neighbourType = neighbour->getType();
									if (neighbourType) {
										neighbourTypes.insert(neighbourType);
									}
									mutex.unlock();
								}
								else {
									neighbour = nodeCache[nHash];
									if (inque.find(nHash) == inque.end() && ready.find(nHash) == ready.end()) {
										queue.push_back(nHash);
										inque.insert(nHash);
									}
								}

								addEdge(current, neighbour, euclidean(current->getPoint(), neighbour->getPoint()));
							}
				}
				
				mutex.lock();
				if (neighbourTypes.empty()) {
					type = getNextType();
					auto set = new unordered_set<int>();

					for (auto node : *area) {
						node.second->setType(type);
						set->insert(node.second->getVoxel());
					}

					areaVoxels->emplace(type, set);
					areas->emplace(type, area);
					area = new unordered_map<size_t, GraphNode*>();
				}
				else {
					type = (*neighbourTypes.begin());
					
					for (int neighbourType : neighbourTypes)
						type = min(type, neighbourType);

					neighbourTypes.erase(type);
					auto areaBefore = areas->at(type);
					auto setBefore = areaVoxels->at(type);

					for (auto node : *area) {
						node.second->setType(type);
						areaBefore->insert(node);
						setBefore->insert(node.second->getVoxel());
					}

					area->clear();

					for (int neighbourType : neighbourTypes) {
						auto neighbourArea = areas->at(neighbourType);
						auto neighbourSet = areaVoxels->at(neighbourType);

						for (auto node : *neighbourArea) {
							node.second->setType(type);
							areaBefore->insert(node);
							setBefore->insert(node.second->getVoxel());
						}

						neighbourSet->clear();
						/*areaVoxels->erase(neighbourType);
						delete neighbourSet;*/

						// TODO: delete neighbourArea from areas
						neighbourArea->clear();
						/*areas->erase(neighbourType);
						delete neighbourArea;*/
					}
				}
				mutex.unlock();
			}

			nodeChanges->clear();
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

		PoolVector3Array navigateToClosestVoxel(Vector3 startV, int voxel, Node* game) {
			boost::unique_lock<boost::mutex> lock(mutex);
			PoolVector3Array path;

			GraphNode* startNode = NULL;

			float minDistanceStart = numeric_limits<float>::max();
			float currDistanceStart;

			for (auto& current : *nodes) {
				currDistanceStart = current.second->getPoint().distance_to(startV);

				if (currDistanceStart < minDistanceStart) {
					minDistanceStart = currDistanceStart;
					startNode = current.second;
				}
			}

			if (!startNode) return path;
			auto voxelSet = areaVoxels->at(startNode->getType());
			if (voxelSet->find(voxel) == voxelSet->end()) {
				Godot::print(String("path from {0} to voxel of type {1} not found").format(Array::make(startNode->getPoint(), voxel)));
				return path;
			}
			
			GraphNode* currentNode;
			GraphNode* neighbourNode;

			Godot::print(String("find path from {0} to closest voxel of type {1}").format(Array::make(startNode->getPoint(), voxel)));

			unordered_map<size_t, size_t> cameFrom;
			unordered_map<size_t, float> costSoFar;
			size_t cHash, nHash, sHash = startNode->getHash();
			int cVoxel;

			PriorityQueue<size_t, float> frontier;
			frontier.put(sHash, 0);
			costSoFar[sHash] = 0;

			while (!frontier.empty()) {
				cHash = frontier.get();
				currentNode = nodes->at(cHash);
				cVoxel = currentNode->getVoxel();

				if (!currentNode) continue;

				if (voxel == cVoxel) {
					Godot::print("path found");

					auto geo = ImmediateGeometry::_new();
					geo->begin(Mesh::PRIMITIVE_LINES);
					geo->set_color(Color(1, 0, 0, 1));

					Vector3 currentPoint = currentNode->getPoint();

					while (true) {
						path.insert(0, currentPoint);
						geo->add_vertex(currentPoint + Vector3(0, 0.25, 0));

						if (cameFrom.find(cHash) == cameFrom.end()) break;

						cHash = cameFrom[cHash];
						currentNode = currentNode->getNeighbour(cHash);
						currentPoint = currentNode->getPoint();

						path.insert(0, currentPoint);
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
						float priority = newCost + 1.0;
						frontier.put(nHash, priority);
						cameFrom[nHash] = cHash;
					}
				}
			}

			return path;
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
			if (startNode->getType() != goalNode->getType()) {
				Godot::print(String("path from {0} to {1} not found").format(Array::make(startNode->getPoint(), goalNode->getPoint())));
				return path;
			}

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
						path.insert(0, currentPoint);
						geo->add_vertex(currentPoint + Vector3(0, 0.25, 0));

						if (cameFrom.find(cHash) == cameFrom.end()) break;

						cHash = cameFrom[cHash];
						currentNode = currentNode->getNeighbour(cHash);
						currentPoint = currentNode->getPoint();

						path.insert(0, currentPoint);
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