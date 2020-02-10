#ifndef NAVIGATOR_H
#define NAVIGATOR_H

#include <Godot.hpp>
#include <Vector3.hpp>
#include <Mesh.hpp>
#include <Texture.hpp>
#include <ImmediateGeometry.hpp>

#include <shared_mutex>
#include <mutex>
#include <deque>
#include <queue>
#include <unordered_set>
#include <unordered_map>
#include <limits>

#include <boost/function.hpp>

#include "constants.h"
#include "fn.h"
#include "graphnode.h"
#include "graphedge.h"

using namespace std;

namespace godot {

	class EcoGame;
	class Chunk;

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

		float manhattan(Vector3 a, Vector3 b);
		float euclidean(Vector3 a, Vector3 b);
		float w(float distanceToGoal, float maxDistance);
		float h(std::shared_ptr<GraphNode> node, std::shared_ptr<GraphNode> goal, float maxDistance);
		std::shared_ptr<GraphNode> getNode(size_t h);
		void setPathActor(PoolVector3Array path, int actorInstanceId, Node* game);

		unordered_map<size_t, std::shared_ptr<GraphNode>>* nodes;
		std::shared_timed_mutex NAV_NODES_MUTEX;
		std::shared_timed_mutex SET_PATH_MUTEX;
	public:
		static std::shared_ptr<Navigator> get() {
			static auto navigator = std::shared_ptr<Navigator>(new Navigator());
			return navigator;
		};

		Navigator();
		~Navigator();

		void addEdge(std::shared_ptr<GraphNode> a, std::shared_ptr<GraphNode> b, float cost);
		void addNode(std::shared_ptr<GraphNode> node);
		void removeNode(std::shared_ptr<GraphNode> node);
		void addNode(std::shared_ptr<GraphNode> node, Chunk* chunk);
		void updateGraph(std::shared_ptr<Chunk> chunk, Node* game);
		void navigateToClosestVoxel(Vector3 startV, int voxel, int actorInstanceId, Node* game, EcoGame* lib);
		void navigate(Vector3 startV, Vector3 goalV, int actorInstanceId, Node* game, EcoGame* lib);
	};

}

#endif