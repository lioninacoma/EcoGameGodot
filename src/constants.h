#ifndef CONSTANTS_H
#define CONSTANTS_H

#define CHUNK_SIZE_X 16
#define CHUNK_SIZE_Y 256
#define CHUNK_SIZE_Z 16
#define VERTEX_SIZE 8
#define BUFFER_SIZE CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z

#define SOUTH  0
#define NORTH  1
#define EAST   2
#define WEST   3
#define TOP    4
#define BOTTOM 5

#define TEXTURE_SCALE 1.0 / 1.0
#define TEXTURE_ATLAS_LEN 3

#define NOISE_SEED 123

#define THREAD_POOL_SIZE 8

#endif