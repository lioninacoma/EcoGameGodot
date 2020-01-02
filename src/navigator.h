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

#include <set>
#include <deque>
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
		typedef boost::property<boost::edge_weight_t, double> EdgeWeightProperty;
		typedef boost::adjacency_list<boost::listS, boost::vecS, boost::undirectedS, boost::no_property, EdgeWeightProperty> UndirectedGraph;
		typedef UndirectedGraph::vertex_descriptor vertex;

		template <class Graph, class CostType, class LocMap>
		class distance_heuristic : public boost::astar_heuristic<Graph, CostType>
		{
		public:
			typedef typename boost::graph_traits<Graph>::vertex_descriptor Vertex;
			distance_heuristic(LocMap l, Vertex goal) : m_location(l), m_goal(goal) {}
			CostType operator()(Vertex u)
			{
				return m_location[m_goal].distance(m_location[u]);
			}
		private:
			LocMap m_location;
			Vertex m_goal;
		};

		struct found_goal {}; // exception for termination

		// visitor that terminates when we find the goal
		template <class Vertex>
		class astar_goal_visitor : public boost::default_astar_visitor
		{
		public:
			astar_goal_visitor(Vertex goal) : m_goal(goal) {}
			template <class Graph>
			void examine_vertex(Vertex u, Graph& g) {
				if (u == m_goal)
					throw found_goal();
			}
		private:
			Vertex m_goal;
		};

		vector<Point>* points;
		unordered_map<size_t, int>* nodes;
		UndirectedGraph* g;
		boost::mutex mutex, graphMutex;
	public:
		static void _register_methods() {
			register_method("updateGraph", &Navigator::updateGraph);
			register_method("navigate", &Navigator::navigate);
		}

		Navigator() {
			points = new vector<Point>();
			nodes = new unordered_map <size_t, int>();
			g = new UndirectedGraph();
		};
		~Navigator() {};

		void _init() {
			
		}

		int addPoint(size_t h, Point p) {
			boost::unique_lock<boost::mutex> lock(mutex);
			int index = points->size();
			nodes->insert(pair<size_t, int>(h, index));
			points->push_back(p);
			return index;
		}

		Point getPoint(int index) {
			boost::unique_lock<boost::mutex> lock(mutex);
			return points->at(index);
		}

		int findPointIndex(size_t hash) {
			boost::unique_lock<boost::mutex> lock(mutex);
			auto it = nodes->find(hash);
			if (it == nodes->end()) return -1;
			return it->second;
		}

		void addEdge(int a, int b, double cost) {
			boost::unique_lock<boost::mutex> lock(graphMutex);
			cout << "add(" << a << ", " << b << ") START" << endl;
			boost::add_edge(a, b, cost, *g);
			cout << "add(" << a << ", " << b << ") END" << endl;
			
		}

		void updateGraph(Chunk* chunk, Node* game) {
			int x, y, z, cIndex, nIndex, drawOffsetY = 1;
			double nx, ny, nz;
			Point current, neighbour;
			auto chunkPoints = chunk->getNodes();
			
			deque<size_t> queue;
			std::set<size_t> ready;

			auto dots = ImmediateGeometry::_new();
			dots->begin(Mesh::PRIMITIVE_POINTS);
			dots->set_color(Color(1, 0, 0, 1));

			for (auto &current : *chunkPoints) {
				dots->add_vertex(Vector3(current.second.x, current.second.y + drawOffsetY, current.second.z));
			}

			dots->end();
			game->call_deferred("draw_debug_dots", dots);

			auto geo = ImmediateGeometry::_new();
			geo->begin(Mesh::PRIMITIVE_LINES);
			geo->set_color(Color(0, 1, 0, 1));

			while (ready.size() != chunkPoints->size()) {
				//Godot::print(String("Chunk{0}: {1}/{2}").format(Array::make(chunk->getOffset(), ready.size(), chunkPoints->size())));

				for (auto &current : *chunkPoints) {
					if (ready.find(current.first) == ready.end()) {
						queue.push_back(current.first);
						addPoint(current.first, current.second);
						//Godot::print(String("add: {0}").format(Array::make(current.first)));
						break;
					}
				}

				while (!queue.empty()) {
					size_t cHash = queue.front();
					queue.pop_front();
					cIndex = findPointIndex(cHash);
					//Godot::print(String("find: {0} = {1}").format(Array::make(cHash, cIndex)));
					if (cIndex < 0) continue;
					current = getPoint(cIndex);
					//Godot::print(String("current: {0}").format(Array::make(Vector3(current.x, current.y, current.z))));
					ready.insert(cHash);

					for (z = -1; z < 2; z++)
						for (x = -1; x < 2; x++)
							for (y = -1; y < 2; y++) {
								if (!x && !y && !z) continue;

								nx = current.x + x;
								ny = current.y + y;
								nz = current.z + z;

								size_t nHash = fn::hash(Point(nx, ny, nz));
								auto it = chunkPoints->find(nHash);

								//if (it == chunkPoints->end()) continue;
								if (it == chunkPoints->end()) {
									nIndex = findPointIndex(nHash);
									if (nIndex < 0) continue;
									// pre existing neighbour node
									neighbour = getPoint(nIndex);
								}
								else {
									if (ready.find(nHash) != ready.end()) continue;

									if (find(queue.begin(), queue.end(), nHash) == queue.end())
										queue.push_back(nHash);

									// neighbour node in new mesh
									neighbour = it->second;

									nIndex = addPoint(nHash, neighbour);
								}

								geo->add_vertex(Vector3(current.x, current.y + drawOffsetY, current.z));
								geo->add_vertex(Vector3(neighbour.x, neighbour.y + drawOffsetY, neighbour.z));

								//if (chunk->getOffset() == Vector3(0, 0, 0))
								cout << "ready(" << cIndex << ", " << nIndex << ") START" << endl;
								addEdge(cIndex, nIndex, 1.0);
							}
				}
			}

			geo->end();
			game->call_deferred("draw_debug", geo);
		}

		PoolVector3Array navigate(Vector3 startV, Vector3 goalV, Node* game) {
			boost::unique_lock<boost::mutex> lock(mutex);
			PoolVector3Array path;

			Point startP = Point(startV.x, startV.y, startV.z);
			Point goalP = Point(goalV.x, goalV.y, goalV.z);
			Point currentP;

			//Godot::print(String("startP: {0}, goalP: {1}").format(Array::make(Vector3(startP.x, startP.y, startP.z), Vector3(goalP.x, goalP.y, goalP.z))));

			float minDistanceStart = numeric_limits<float>::max();
			float minDistanceGoal = numeric_limits<float>::max();
			float currDistanceStart;
			float currDistanceGoal;
			int startIndex = -1, goalIndex = -1;

			for (int i = 0; i < points->size(); i++) {
				currentP = points->at(i);
				currDistanceStart = currentP.distance(startP);
				currDistanceGoal = currentP.distance(goalP);

				if (currDistanceStart < minDistanceStart) {
					minDistanceStart = currDistanceStart;
					startIndex = i;
				}

				if (currDistanceGoal < minDistanceGoal) {
					minDistanceGoal = currDistanceGoal;
					goalIndex = i;
				}
			}

			if (startIndex < 0 || goalIndex < 0) return path;
			startP = points->at(startIndex);
			goalP = points->at(goalIndex);

			size_t sHash = fn::hash(startP);
			size_t gHash = fn::hash(goalP);

			auto startIt = nodes->find(sHash);
			if (startIt == nodes->end()) return path;

			auto goalIt = nodes->find(gHash);
			if (goalIt == nodes->end()) return path;

			int start = startIt->second;
			int goal = goalIt->second;

			if (boost::num_vertices(*g) == 0) return path;

			vector<vertex> p(boost::num_vertices(*g));
			vector<double> d(boost::num_vertices(*g));

			try {
				// call astar named parameter interface
				Godot::print(String("find path from {0} to {1}").format(Array::make(Vector3(startP.x, startP.y, startP.z), Vector3(goalP.x, goalP.y, goalP.z))));
				boost::astar_search_tree(*g, start,
					distance_heuristic<UndirectedGraph, double, vector<Point>>(*points, goal),
					boost::predecessor_map(boost::make_iterator_property_map(p.begin(), boost::get(boost::vertex_index, *g))).
					distance_map(boost::make_iterator_property_map(d.begin(), boost::get(boost::vertex_index, *g))).
					visitor(astar_goal_visitor<vertex>(goal)));
			}
			catch (found_goal fg) { // found a path to the goal
				return path;
				list<vertex> shortest_path;
				for (vertex v = goal;; v = p[v]) {
					shortest_path.push_front(v);
					if (p[v] == v)
						break;
				}

				list<vertex>::iterator spi = shortest_path.begin();
				Vector3 a, b;
				int spi_b = start;
				auto geo = ImmediateGeometry::_new();
				geo->begin(Mesh::PRIMITIVE_LINES);
				geo->set_color(Color(1, 0, 0, 1));
				cout << start << " ";

				a = Vector3(points->at(start).x, points->at(start).y, points->at(start).z);
				path.append(a);

				for (++spi; spi != shortest_path.end(); ++spi) {
					cout << *spi << " ";
					
					a = Vector3(points->at(spi_b).x, points->at(spi_b).y, points->at(spi_b).z);
					b = Vector3(points->at(*spi).x, points->at(*spi).y, points->at(*spi).z);

					geo->add_vertex(a);
					geo->add_vertex(b);

					path.append(b);

					spi_b = *spi;
				}
				cout << endl;

				geo->end();
				game->call_deferred("draw_debug", geo);
			}

			return path;
		}
	};

}

#endif