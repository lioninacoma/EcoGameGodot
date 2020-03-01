#ifndef CONSTANTS_H
#define CONSTANTS_H

// ~5 GB
//#define WORLD_SIZE 40	   // Chunks
//#define CHUNK_SIZE_X 32  // Voxels
//#define CHUNK_SIZE_Y 128 // Voxels
//#define CHUNK_SIZE_Z 32  // Voxels
//#define SECTION_SIZE 4   // Chunks

#define WORLD_SIZE 8	  // Chunks
#define CHUNK_SIZE_X 8   // Voxels
#define CHUNK_SIZE_Y 64  // Voxels
#define CHUNK_SIZE_Z 8   // Voxels
#define SECTION_SIZE 1    // Chunks

//#define WORLD_SIZE 8	   // Chunks
//#define CHUNK_SIZE_X 32  // Voxels
//#define CHUNK_SIZE_Y 128 // Voxels
//#define CHUNK_SIZE_Z 32  // Voxels
//#define SECTION_SIZE 4   // Chunks

#define VERTEX_SIZE 8
#define BUFFER_SIZE CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z
#define MAX_VERTICES_SIZE (BUFFER_SIZE * VERTEX_SIZE * 6 * 4) / 2
#define SECTIONS_SIZE WORLD_SIZE / SECTION_SIZE
#define SECTION_CHUNKS_LEN SECTION_SIZE * SECTION_SIZE 
#define SECTIONS_LEN SECTIONS_SIZE * SECTIONS_SIZE

#define INT_POOL_BUFFER_SIZE CHUNK_SIZE_X * CHUNK_SIZE_Z * SECTION_CHUNKS_LEN

#define TEXTURE_SCALE 1.0 / 1.0
#define TEXTURE_ATLAS_LEN 3

#define TYPES 8
#define NOISE_SEED 123
#define WATER_LEVEL 48
#define VOXEL_CHANCE_T 1.0
#define VOXEL_CHANCE_NOISE_SCALE 1.0
#define VOXEL_Y_NOISE_SCALE 1.0

#define POOL_SIZE 4
#define NAV_POOL_SIZE 16
#define MAX_CHUNKS_BUILT_ASYNCH 2

// navigator
#define MAX_WEIGHT 5.0
#define MAX_WEIGHT_ROOT sqrt(MAX_WEIGHT)

#endif