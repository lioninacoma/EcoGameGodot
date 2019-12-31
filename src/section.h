#ifndef SECTION_H
#define SECTION_H

#include <Godot.hpp>
#include <Reference.hpp>
#include <Vector3.hpp>
#include <Texture.hpp>
#include <ImmediateGeometry.hpp>

#include <iostream>
#include <string>
#include <unordered_map>

#include <boost/graph/astar_search.hpp>
#include <boost/graph/adjacency_list.hpp>
//#include <boost/geometry.hpp>
//#include <boost/geometry/geometries/polygon.hpp>
#include <boost/polygon/voronoi.hpp>

#include "voxelassetmanager.h"
#include "constants.h"
#include "chunkbuilder.h"
#include "fn.h"
#include "chunk.h"

//namespace bg = boost::geometry;

//namespace boost {
//	namespace polygon {
//		template <>
//		struct geometry_concept<Point> {
//			typedef point_concept type;
//		};
//
//		template <>
//		struct point_traits<Point> {
//			typedef double coordinate_type;
//
//			static inline coordinate_type get(const Point& point, orientation_2d orient) {
//				return (orient == HORIZONTAL) ? point.x : point.z;
//			}
//		};
//	}
//}

using boost::polygon::voronoi_diagram;
using namespace std;

namespace godot {

	class Section : public Reference {
		GODOT_CLASS(Section, Reference)

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

		UndirectedGraph* graph;
		unordered_map<std::size_t, Point>* nodes;
		Chunk** chunks;
		Vector2 offset;
	public:
		static ObjectPool<int, INT_POOL_BUFFER_SIZE, POOL_SIZE * 4>& getIntBufferPool() {
			static ObjectPool<int, INT_POOL_BUFFER_SIZE, POOL_SIZE * 4> pool;
			return pool;
		};

		static void _register_methods() {
			register_method("getOffset", &Section::getOffset);
		}

		Section() : Section(Vector2()) {};
		Section(Vector2 offset) {
			Section::offset = offset;
			chunks = new Chunk * [SECTION_CHUNKS_LEN * sizeof(*chunks)];
			graph = new UndirectedGraph();
			nodes = new unordered_map<std::size_t, Point>();

			memset(chunks, 0, SECTION_CHUNKS_LEN * sizeof(*chunks));
		}

		~Section() {
			delete[] chunks;
		}

		// our initializer called by Godot
		void _init() {

		}

		void build(ChunkBuilder* builder, Node* game) {
			int x, y, i;
			Chunk* chunk;

			for (y = 0; y < SECTION_SIZE; y++) {
				for (x = 0; x < SECTION_SIZE; x++) {
					chunk = Chunk::_new();
					chunk->setOffset(Vector3((x + offset.x) * CHUNK_SIZE_X, 0, (y + offset.y) * CHUNK_SIZE_Z));
					chunks[fn::fi2(x, y, SECTION_SIZE)] = chunk;
					chunk->buildVolume();
				}
			}

			buildAreasByType(VoxelAssetType::HOUSE_6X6);
			buildAreasByType(VoxelAssetType::HOUSE_4X4);
			buildAreasByType(VoxelAssetType::PINE_TREE);

			//buildGraph(game);
			
			for (i = 0; i < SECTION_CHUNKS_LEN; i++) {
				chunk = chunks[i];
				if (!chunk) continue;
				builder->build(chunk, game);
			}
		}

		Vector2 getOffset() {
			return Section::offset;
		}

		Chunk* getChunk(int x, int y) {
			if (x < 0 || x >= SECTION_SIZE) return NULL;
			if (y < 0 || y >= SECTION_SIZE) return NULL;
			return chunks[fn::fi2(x, y, SECTION_SIZE)];
		}

		Chunk* getChunk(int i) {
			if (i < 0 || i >= SECTION_CHUNKS_LEN) return NULL;
			return chunks[i];
		}

		//void buildGraph(Node* game) {
		//	int y0, y1, y2, areaSize;
		//	int drawOffsetY = 1;
		//	int maxDeltaY = 0;
		//	//double maxDst = 5.0;
		//	double m, o;

		//	int currentY, currentType, currentIndex, deltaY, ci, i, j, it, vx, vz, maxArea, currentArea;
		//	Vector2 areasOffset;
		//	Vector3 chunkOffset;
		//	Point* node;
		//	Chunk* chunk;
		//	Area* nextArea;
		//	vector<int> yValues;

		//	areasOffset.x = offset.x * CHUNK_SIZE_X; // Voxel
		//	areasOffset.y = offset.y * CHUNK_SIZE_Z; // Voxel

		//	//Godot::print(String("areasOffset: {0}").format(Array::make(areasOffset)));
		//	//return;

		//	int* surfaceY = getIntBufferPool().borrow();
		//	memset(surfaceY, 0, INT_POOL_BUFFER_SIZE * sizeof(*surfaceY));

		//	int* mask = getIntBufferPool().borrow();
		//	memset(mask, 0, INT_POOL_BUFFER_SIZE * sizeof(*mask));

		//	int* types = getIntBufferPool().borrow();
		//	memset(types, 0, INT_POOL_BUFFER_SIZE * sizeof(*types));

		//	//Godot::print(String("chunks, surfaceY and mask initialized"));

		//	const int SECTION_SIZE_CHUNKS = SECTION_SIZE * CHUNK_SIZE_X;

		//	for (it = 0; it < SECTION_CHUNKS_LEN; it++) {
		//		chunk = chunks[it];

		//		if (!chunk) continue;

		//		chunkOffset = chunk->getOffset();				// Voxel
		//		chunkOffset = fn::toChunkCoords(chunkOffset);	// Chunks

		//		//Godot::print(String("chunkOffset: {0}").format(Array::make(chunkOffset)));

		//		for (j = 0; j < CHUNK_SIZE_Z; j++) {
		//			for (i = 0; i < CHUNK_SIZE_X; i++) {
		//				vx = (int)((chunkOffset.x - offset.x) * CHUNK_SIZE_X + i);
		//				vz = (int)((chunkOffset.z - offset.y) * CHUNK_SIZE_Z + j);

		//				//Godot::print(String("vPos: {0}").format(Array::make(vPos)));

		//				currentY = chunk->getCurrentSurfaceY(i, j);
		//				currentType = chunk->getVoxel(i, currentY, j);
		//				currentIndex = fn::fi2(vx, vz, SECTION_SIZE_CHUNKS);
		//				yValues.push_back(currentY);
		//				surfaceY[currentIndex] = currentY;
		//				types[currentIndex] = currentType;
		//			}
		//		}
		//	}

		//	std::sort(yValues.begin(), yValues.end(), std::greater<int>());
		//	auto last = std::unique(yValues.begin(), yValues.end());
		//	yValues.erase(last, yValues.end());

		//	//Godot::print(String("surfaceY and uniques set"));
		//	int lastY = -1, nextAreaSize;
		//	currentArea = 0;
		//	maxArea = 0;

		//	for (int nextY : yValues) {

		//		if (lastY >= 0 && lastY - nextY <= maxDeltaY) continue;
		//		lastY = nextY;

		//		//Godot::print(String("nextY: {0}").format(Array::make(nextY)));

		//		for (i = 0; i < SECTION_SIZE_CHUNKS * SECTION_SIZE_CHUNKS; i++) {
		//			deltaY = nextY - surfaceY[i];
		//			currentType = types[i];
		//			mask[i] = (deltaY <= maxDeltaY && deltaY >= 0 && currentType != 6) ? 1 : 0;
		//			maxArea += mask[i];
		//		}

		//		do {
		//			nextArea = findNextArea(mask, surfaceY);
		//			nextAreaSize = nextArea->getWidth() * nextArea->getHeight();
		//			currentArea += nextAreaSize;
		//			for (j = (int)nextArea->getStart().y; j < (int)nextArea->getEnd().y; j++) {
		//				for (i = (int)nextArea->getStart().x; i < (int)nextArea->getEnd().x; i++) {
		//					mask[fn::fi2(i, j)] = 0;
		//				}
		//			}

		//			//if (nextAreaSize <= 1) break;
		//			nextArea->setOffset(areasOffset);
		//			node = getNode(*nextArea);
		//			nodes->insert(pair<std::size_t, Point>(fn::hash(*node), *node));
		//		} while (currentArea < maxArea);
		//		
		//	}

		//	getIntBufferPool().ret(surfaceY);
		//	getIntBufferPool().ret(mask);
		//	getIntBufferPool().ret(types);

		//	vector<Point> points;
		//	for (const auto& n : *nodes) {
		//		points.push_back(n.second);
		//	}

		//	if (points.empty()) return;

		//	UndirectedGraph g;
		//	voronoi_diagram<double> vd;
		//	construct_voronoi(points.begin(), points.end(), &vd);

		//	auto geo = ImmediateGeometry::_new();
		//	geo->begin(Mesh::PRIMITIVE_LINES);
		//	geo->set_color(Color(0, 1, 0, 1));

		//	for (const auto& vertex : vd.vertices()) {
		//		std::vector<int> triangle;
		//		auto edge = vertex.incident_edge();
		//		do {
		//			auto cell = edge->cell();
		//			assert(cell->contains_point());

		//			triangle.push_back(cell->source_index());
		//			if (triangle.size() == 3) {
		//				// process output triangles
		//				y0 = points[triangle[0]].y;
		//				y1 = points[triangle[1]].y;
		//				y2 = points[triangle[2]].y;
		//				m = (y0 + y1 + y2) / 3.0;
		//				o = ((y0 * y0 - m * m) + (y1 * y1 - m * m) + (y2 * y2 - m * m)) / 3.0;

		//				if (o <= 0.25) {

		//					//buildNode(points[triangle[0]]);
		//					//buildNode(points[triangle[1]]);
		//					//buildNode(points[triangle[2]]);

		//					geo->add_vertex(Vector3(points[triangle[0]].x, points[triangle[0]].y + drawOffsetY, points[triangle[0]].z));
		//					geo->add_vertex(Vector3(points[triangle[1]].x, points[triangle[1]].y + drawOffsetY, points[triangle[1]].z));
		//					geo->add_vertex(Vector3(points[triangle[1]].x, points[triangle[1]].y + drawOffsetY, points[triangle[1]].z));
		//					geo->add_vertex(Vector3(points[triangle[2]].x, points[triangle[2]].y + drawOffsetY, points[triangle[2]].z));
		//					geo->add_vertex(Vector3(points[triangle[2]].x, points[triangle[2]].y + drawOffsetY, points[triangle[2]].z));
		//					geo->add_vertex(Vector3(points[triangle[0]].x, points[triangle[0]].y + drawOffsetY, points[triangle[0]].z));

		//					boost::add_edge(triangle[0], triangle[1], abs(y0 - y1), g);
		//					boost::add_edge(triangle[1], triangle[2], abs(y1 - y2), g);
		//					boost::add_edge(triangle[2], triangle[0], abs(y2 - y0), g);
		//				}

		//				triangle.erase(triangle.begin() + 1);
		//			}

		//			edge = edge->rot_next();
		//		} while (edge != vertex.incident_edge());
		//	}

		//	geo->end();
		//	game->call_deferred("draw_debug", geo);
		//	return;
		//	if (boost::num_vertices(g) == 0) return;

		//	int start = 0;
		//	int goal = 1;

		//	vector<vertex> p(boost::num_vertices(g));
		//	vector<double> d(boost::num_vertices(g));

		//	geo = ImmediateGeometry::_new();
		//	geo->begin(Mesh::PRIMITIVE_LINES);
		//	geo->set_color(Color(1, 0, 0, 1));

		//	try {
		//		// call astar named parameter interface
		//		boost::astar_search_tree(g, start,
		//			distance_heuristic<UndirectedGraph, double, vector<Point>>(points, goal),
		//			boost::predecessor_map(boost::make_iterator_property_map(p.begin(), boost::get(boost::vertex_index, g))).
		//			distance_map(boost::make_iterator_property_map(d.begin(), boost::get(boost::vertex_index, g))).
		//			visitor(astar_goal_visitor<vertex>(goal)));
		//	}
		//	catch (found_goal fg) { // found a path to the goal
		//		list<vertex> shortest_path;
		//		for (vertex v = goal;; v = p[v]) {
		//			shortest_path.push_front(v);
		//			if (p[v] == v)
		//				break;
		//		}
		//		list<vertex>::iterator spi = shortest_path.begin();
		//		int spi_b = start;
		//		for (++spi; spi != shortest_path.end(); ++spi) {
		//			geo->add_vertex(Vector3(points[spi_b].x, points[spi_b].y + drawOffsetY, points[spi_b].z));
		//			geo->add_vertex(Vector3(points[*spi].x, points[*spi].y + drawOffsetY, points[*spi].z));
		//			spi_b = *spi;
		//		}
		//	}
		//	
		//	geo->end();
		//	game->call_deferred("draw_debug", geo);
		//}

		void buildAreasByType(VoxelAssetType type) {
			VoxelAsset* voxelAsset = VoxelAssetManager::get()->getVoxelAsset(type);
			int maxDeltaY = voxelAsset->getMaxDeltaY();
			int areaSize = max(voxelAsset->getWidth(), voxelAsset->getHeight());
			int currentY, currentType, currentIndex, deltaY, lastY = -1, ci, i, j, it, vx, vz;
			Vector2 areasOffset;
			Vector3 chunkOffset;
			Chunk* chunk;
			vector<Area> areas;
			vector<int> yValues;

			areasOffset.x = offset.x * CHUNK_SIZE_X; // Voxel
			areasOffset.y = offset.y * CHUNK_SIZE_Z; // Voxel

			//Godot::print(String("areasOffset: {0}").format(Array::make(areasOffset)));
			//return;

			int* surfaceY = getIntBufferPool().borrow();
			memset(surfaceY, 0, INT_POOL_BUFFER_SIZE * sizeof(*surfaceY));

			int* mask = getIntBufferPool().borrow();
			memset(mask, 0, INT_POOL_BUFFER_SIZE * sizeof(*mask));

			int* types = getIntBufferPool().borrow();
			memset(types, 0, INT_POOL_BUFFER_SIZE * sizeof(*types));

			//Godot::print(String("chunks, surfaceY and mask initialized"));

			const int SECTION_SIZE_CHUNKS = SECTION_SIZE * CHUNK_SIZE_X;

			for (it = 0; it < SECTION_CHUNKS_LEN; it++) {
				chunk = chunks[it];

				if (!chunk) continue;

				chunkOffset = chunk->getOffset();				// Voxel
				chunkOffset = fn::toChunkCoords(chunkOffset);	// Chunks

				//Godot::print(String("chunkOffset: {0}").format(Array::make(chunkOffset)));

				for (j = 0; j < CHUNK_SIZE_Z; j++) {
					for (i = 0; i < CHUNK_SIZE_X; i++) {
						vx = (int)((chunkOffset.x - offset.x) * CHUNK_SIZE_X + i);
						vz = (int)((chunkOffset.z - offset.y) * CHUNK_SIZE_Z + j);

						//Godot::print(String("vPos: {0}").format(Array::make(vPos)));

						currentY = chunk->getCurrentSurfaceY(i, j);
						currentType = chunk->getVoxel(i, currentY, j);
						currentIndex = fn::fi2(vx, vz, SECTION_SIZE_CHUNKS);
						yValues.push_back(currentY);
						surfaceY[currentIndex] = currentY;
						types[currentIndex] = currentType;
					}
				}
			}

			std::sort(yValues.begin(), yValues.end(), std::greater<int>());
			auto last = std::unique(yValues.begin(), yValues.end());
			yValues.erase(last, yValues.end());

			//Godot::print(String("surfaceY and uniques set"));

			for (int nextY : yValues) {

				if (lastY >= 0 && lastY - nextY <= maxDeltaY) continue;
				lastY = nextY;

				//Godot::print(String("nextY: {0}").format(Array::make(nextY)));

				for (i = 0; i < SECTION_SIZE_CHUNKS * SECTION_SIZE_CHUNKS; i++) {
					deltaY = nextY - surfaceY[i];
					currentType = types[i];
					mask[i] = (deltaY <= maxDeltaY && deltaY >= 0 && currentType != 6) ? 1 : 0;
				}

				areas = findAreasOfSize(areaSize, mask, surfaceY);

				for (Area area : areas) {
					area.setOffset(areasOffset);
					buildArea(area, type);
					//Godot::print(String("area start: {0}, end: {1}").format(Array::make(area.getStart(), area.getEnd())));
				}
			}

			getIntBufferPool().ret(surfaceY);
			getIntBufferPool().ret(mask);
			getIntBufferPool().ret(types);
		}

		/*void buildNode(Point p) {
			int voxelType, vx, vy, vz, ci, cx, cz;
			Chunk* chunk;
			Vector2 chunkOffset;
			Vector3 pos = Vector3(p.x, p.y, p.z);

			voxelType = 2;
			chunkOffset.x = pos.x;
			chunkOffset.y = pos.z;
			chunkOffset = fn::toChunkCoords(chunkOffset);
			cx = (int)(chunkOffset.x - offset.x);
			cz = (int)(chunkOffset.y - offset.y);
			ci = fn::fi2(cx, cz, SECTION_SIZE);

			if (ci < 0 || ci >= SECTION_CHUNKS_LEN) return;

			chunk = chunks[ci];

			if (!chunk) return;

			vx = (int)pos.x;
			vy = (int)pos.y;
			vz = (int)pos.z;

			chunk->setVoxel(
				vx % CHUNK_SIZE_X,
				vy % CHUNK_SIZE_Y,
				vz % CHUNK_SIZE_Z, voxelType);
		}*/

		/*Point* getNode(int x, int z) {
			int vx, vy, vz, ci, cx, cz;
			Chunk* chunk;
			Vector2 chunkOffset;
			Vector3 pos = Vector3(x, 0, z);

			chunkOffset.x = pos.x;
			chunkOffset.y = pos.z;
			chunkOffset = fn::toChunkCoords(chunkOffset);
			cx = (int)(chunkOffset.x - offset.x);
			cz = (int)(chunkOffset.y - offset.y);
			ci = fn::fi2(cx, cz, SECTION_SIZE);

			if (ci < 0 || ci >= SECTION_CHUNKS_LEN) return NULL;

			chunk = chunks[ci];

			if (!chunk) return NULL;

			vx = (int)pos.x;
			vz = (int)pos.z;
			vy = (int)chunk->getCurrentSurfaceY(vx % CHUNK_SIZE_X, vz % CHUNK_SIZE_Z) + 1;

			return new Point(vx, vy, vz);
		}

		Point* getNode(Area area) {
			Vector2 start = area.getStart() + area.getOffset();
			return getNode(start.x + (area.getWidth() / 2.0), start.y + (area.getHeight() / 2.0));
		}*/

		void buildArea(Area area, VoxelAssetType type) {
			int voxelType, vx, vy, vz, ci, cx, cz, ay = area.getY() + 1;
			Chunk* chunk;
			Vector2 chunkOffset;
			Vector2 start = area.getStart() + area.getOffset();
			vector<Voxel>* voxels = VoxelAssetManager::get()->getVoxels(type);

			if (!voxels) return;

			Vector3 pos;
			Vector3 voxelOffset = Vector3(start.x, ay, start.y);

			for (vector<Voxel>::iterator it = voxels->begin(); it != voxels->end(); it++) {
				Voxel v = *it;
				pos = v.getPosition();
				pos += voxelOffset;
				voxelType = v.getType();
				chunkOffset.x = pos.x;
				chunkOffset.y = pos.z;
				chunkOffset = fn::toChunkCoords(chunkOffset);
				cx = (int)(chunkOffset.x - offset.x);
				cz = (int)(chunkOffset.y - offset.y);
				ci = fn::fi2(cx, cz, SECTION_SIZE);

				if (ci < 0 || ci >= SECTION_CHUNKS_LEN) continue;

				chunk = chunks[ci];

				if (!chunk) continue;

				vx = (int)pos.x;
				vy = (int)pos.y;
				vz = (int)pos.z;

				chunk->setVoxel(
					vx % CHUNK_SIZE_X,
					vy % CHUNK_SIZE_Y,
					vz % CHUNK_SIZE_Z, voxelType);

				chunk->markAssetsBuilt();
			}
		}

		Area* findNextArea(int* mask, int* surfaceY) {
			const int SECTION_SIZE_CHUNKS = SECTION_SIZE * CHUNK_SIZE_X;
			int i, j, x0, x1, y0, y1;
			int max_of_s, max_i, max_j;

			int* sub = getIntBufferPool().borrow();

			memset(sub, 0, INT_POOL_BUFFER_SIZE * sizeof(*sub));

			for (i = 0; i < SECTION_SIZE_CHUNKS; i++)
				sub[fn::fi2(i, 0, SECTION_SIZE_CHUNKS)] = mask[fn::fi2(i, 0, SECTION_SIZE_CHUNKS)];

			for (j = 0; j < SECTION_SIZE_CHUNKS; j++)
				sub[fn::fi2(0, j, SECTION_SIZE_CHUNKS)] = mask[fn::fi2(0, j, SECTION_SIZE_CHUNKS)];

			for (i = 1; i < SECTION_SIZE_CHUNKS; i++) {
				for (j = 1; j < SECTION_SIZE_CHUNKS; j++) {
					if (mask[fn::fi2(i, j, SECTION_SIZE_CHUNKS)] > 0) {
						sub[fn::fi2(i, j, SECTION_SIZE_CHUNKS)] = min(
							sub[fn::fi2(i, j - 1, SECTION_SIZE_CHUNKS)],
							min(sub[fn::fi2(i - 1, j, SECTION_SIZE_CHUNKS)],
								sub[fn::fi2(i - 1, j - 1, SECTION_SIZE_CHUNKS)])) + 1;
					}
					else {
						sub[fn::fi2(i, j, SECTION_SIZE_CHUNKS)] = 0;
					}
				}
			}

			max_of_s = sub[fn::fi2(0, 0)];
			max_i = 0;
			max_j = 0;

			for (i = 0; i < SECTION_SIZE_CHUNKS; i++) {
				for (j = 0; j < SECTION_SIZE_CHUNKS; j++) {
					if (max_of_s < sub[fn::fi2(i, j)]) {
						max_of_s = sub[fn::fi2(i, j)];
						max_i = i;
						max_j = j;
					}
				}
			}

			x0 = (max_i - max_of_s) + 1;
			x1 = max_i + 1;
			y0 = (max_j - max_of_s) + 1;
			y1 = max_j + 1;
			
			getIntBufferPool().ret(sub);

			return new Area(Vector2(x0, y0), Vector2(x1, y1), surfaceY[fn::fi2(x0, y0)]);
		}

		vector<Area> findAreasOfSize(int size, int* mask, int* surfaceY) {
			int i, j, x, y, x0, x1, y0, y1, areaY, currentSize, meanY = 0, count = 0;
			vector<Area> areas;

			const int SECTION_SIZE_CHUNKS = SECTION_SIZE * CHUNK_SIZE_X;

			int* sub = getIntBufferPool().borrow();
			memset(sub, 0, INT_POOL_BUFFER_SIZE * sizeof(*sub));

			for (i = 0; i < SECTION_SIZE_CHUNKS; i++)
				sub[fn::fi2(i, 0, SECTION_SIZE_CHUNKS)] = mask[fn::fi2(i, 0, SECTION_SIZE_CHUNKS)];

			for (j = 0; j < SECTION_SIZE_CHUNKS; j++)
				sub[fn::fi2(0, j, SECTION_SIZE_CHUNKS)] = mask[fn::fi2(0, j, SECTION_SIZE_CHUNKS)];

			for (i = 1; i < SECTION_SIZE_CHUNKS; i++) {
				for (j = 1; j < SECTION_SIZE_CHUNKS; j++) {
					if (mask[fn::fi2(i, j, SECTION_SIZE_CHUNKS)] > 0) {
						sub[fn::fi2(i, j, SECTION_SIZE_CHUNKS)] = min(
							sub[fn::fi2(i, j - 1, SECTION_SIZE_CHUNKS)],
							min(sub[fn::fi2(i - 1, j, SECTION_SIZE_CHUNKS)],
								sub[fn::fi2(i - 1, j - 1, SECTION_SIZE_CHUNKS)])) + 1;

						currentSize = sub[fn::fi2(i, j, SECTION_SIZE_CHUNKS)];

						if (currentSize == size) {
							x0 = (i - currentSize) + 1;
							y0 = (j - currentSize) + 1;
							x1 = i + 1;
							y1 = j + 1;

							meanY = 0;
							count = 0;

							for (y = y0; y < y1; y++) {
								for (x = x0; x < x1; x++) {
									sub[fn::fi2(x, y, SECTION_SIZE_CHUNKS)] = 0;
									meanY += surfaceY[fn::fi2(x, y, SECTION_SIZE_CHUNKS)];
									count++;
								}
							}

							areaY = (int)((float)meanY / (float)count);
							areas.push_back(Area(Vector2(x0, y0), Vector2(x1, y1), areaY));
						}
					}
					else {
						sub[fn::fi2(i, j, SECTION_SIZE_CHUNKS)] = 0;
					}
				}
			}

			getIntBufferPool().ret(sub);
			return areas;
		}
	};
}

#endif