#ifndef CHUNKBUILDER_H
#define CHUNKBUILDER_H

#include <Spatial.hpp>
#include <String.hpp>
#include <Array.hpp>
#include <MeshInstance.hpp>
#include <StaticBody.hpp>
#include <CollisionShape.hpp>
#include <ConcavePolygonShape.hpp>
#include <SurfaceTool.hpp>
#include <ArrayMesh.hpp>
#include <ImmediateGeometry.hpp>

#include <set>
#include <deque>
#include <vector>
#include <iostream>

#include <boost/graph/adjacency_list.hpp>
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/polygon/voronoi.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include "constants.h"
#include "chunk.h"
#include "meshbuilder.h"
#include "objectpool.h"
#include "threadpool.h"

namespace bg = boost::geometry;

namespace boost {
	namespace polygon {
		template <>
		struct geometry_concept<Point> {
			typedef point_concept type;
		};

		template <>
		struct point_traits<Point> {
			typedef double coordinate_type;

			static inline coordinate_type get(const Point& point, orientation_2d orient) {
				return (orient == HORIZONTAL) ? point.x : point.z;
			}
		};
	}
}

using boost::polygon::voronoi_diagram;
using namespace std;
namespace bpt = boost::posix_time;

namespace godot {

	class ChunkBuilder {

	private :
		typedef boost::property<boost::edge_weight_t, double> EdgeWeightProperty;
		typedef boost::adjacency_list<boost::listS, boost::vecS, boost::undirectedS, boost::no_property, EdgeWeightProperty> UndirectedGraph;
		typedef UndirectedGraph::vertex_descriptor vertex;

		class Worker {
		private:
			static ObjectPool<float, MAX_VERTICES_SIZE, 16>& getVerticesPool() {
				static ObjectPool<float, MAX_VERTICES_SIZE, 16> pool;
				return pool;
			};
			MeshBuilder meshBuilder;
		public:
			void run(Chunk* chunk, Node* game);
		};
	public:
		ChunkBuilder() {};
		~ChunkBuilder() {};
		void build(Chunk* chunk, Node* game);
	};

}

#endif