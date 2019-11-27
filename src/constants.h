#ifndef CONSTANTS_H
#define CONSTANTS_H

#define WORLD_SIZE 8
#define CHUNK_SIZE_X 16
#define CHUNK_SIZE_Y 96
#define CHUNK_SIZE_Z 16
#define VERTEX_SIZE 8
#define BUFFER_SIZE CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z
#define MAX_VERTICES_SIZE BUFFER_SIZE * VERTEX_SIZE * 6 * 4

#define SOUTH  0
#define NORTH  1
#define EAST   2
#define WEST   3
#define TOP    4
#define BOTTOM 5

#define TEXTURE_SCALE 1.0 / 1.0
#define TEXTURE_ATLAS_LEN 3

#define NOISE_SEED 123
#define VOXEL_CHANCE_T 0.7
#define VOXEL_CHANCE_NOISE_SCALE 1.0
#define VOXEL_Y_NOISE_SCALE 0.25

#define POOL_SIZE 8

#endif