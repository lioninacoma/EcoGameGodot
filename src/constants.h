#ifndef CONSTANTS_H
#define CONSTANTS_H

#define WORLD_SIZE 64	// Chunks
#define CHUNK_SIZE_X 32 // Voxels
#define CHUNK_SIZE_Y 80 // Voxels
#define CHUNK_SIZE_Z 32 // Voxels
#define VERTEX_SIZE 8
#define BUFFER_SIZE CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z
#define MAX_VERTICES_SIZE (BUFFER_SIZE * VERTEX_SIZE * 6 * 4) / 2

#define SECTION_SIZE 1 // Chunks
#define SECTIONS_SIZE WORLD_SIZE / SECTION_SIZE
#define SECTION_CHUNKS_LEN SECTION_SIZE * SECTION_SIZE 
#define SECTIONS_LEN SECTIONS_SIZE * SECTIONS_SIZE

#define INT_POOL_BUFFER_SIZE CHUNK_SIZE_X * CHUNK_SIZE_Z * SECTION_CHUNKS_LEN

#define TEXTURE_SCALE 1.0 / 1.0
#define TEXTURE_ATLAS_LEN 3

#define NOISE_SEED 123
#define WATER_LEVEL 25
#define VOXEL_CHANCE_T 0.7
#define VOXEL_CHANCE_NOISE_SCALE 1.0
#define VOXEL_Y_NOISE_SCALE 0.33

#define MAX_BUILD_AREAS 64

#define POOL_SIZE 8

#define CGAL_EIGEN3_ENABLED

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Triangulation_3.h>

typedef CGAL::Exact_predicates_inexact_constructions_kernel K;

typedef CGAL::Triangulation_3<K>      Triangulation;

typedef Triangulation::Cell_handle    Cell_handle;
typedef Triangulation::Vertex_handle  Vertex_handle;
typedef Triangulation::Locate_type    Locate_type;
typedef Triangulation::Point          Point;

//struct Point {
//	double x;
//	double y;
//	double z;
//	Point() : Point(0.0, 0.0, 0.0) {}
//	Point(double x, double y, double z) : x(x), y(y), z(z) {}
//	int distance(Point o) {
//		return sqrt(pow(x - o.x, 2.0) + pow(y - o.y, 2.0) + pow(z - o.z, 2.0));
//	}
//};

#endif