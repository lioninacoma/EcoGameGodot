#ifndef NAVIGATOR_H
#define NAVIGATOR_H

#include <Godot.hpp>
#include <Vector3.hpp>
#include <Mesh.hpp>
#include <Texture.hpp>
#include <ImmediateGeometry.hpp>

#include <deque>
#include <queue>
#include <unordered_set>
#include <unordered_map>
#include <limits>

#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/function.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include "constants.h"
#include "fn.h"

using namespace std;

namespace godot {

	class GraphNavNode;
	class GraphEdge;
	class EcoGame;
	class VoxelWorld;
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

		float w(float distanceToGoal, float maxDistance);
		float h(std::shared_ptr<GraphNavNode> node, std::shared_ptr<GraphNavNode> goal, float maxDistance);
		void setPathActor(Array path, int actorInstanceId);

		std::shared_ptr<VoxelWorld> world;
	public:
		Navigator(std::shared_ptr<VoxelWorld> world);
		~Navigator();

		void navigate(Vector3 startV, Vector3 goalV, int actorInstanceId);
	};

}

#endif