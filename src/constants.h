#ifndef CONSTANTS_H
#define CONSTANTS_H

#define CHUNK_SIZE 16    // Voxels

#define VERTEX_SIZE 8
#define BUFFER_SIZE CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE
#define MAX_VERTICES_SIZE (BUFFER_SIZE * VERTEX_SIZE * 6 * 4) / 2

#define NOISE_SEED 123

#define POOL_SIZE 16
#define NAV_POOL_SIZE 32

// navigator
#define MAX_WEIGHT 1.0
#define MAX_WEIGHT_ROOT sqrt(MAX_WEIGHT)

#endif