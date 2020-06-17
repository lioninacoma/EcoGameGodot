#include "meshbuilder.h"
#include "graphnode.h"
#include "navigator.h"
#include "ThreadPool.h"
#include "voxelworld.h"

using namespace godot;

MeshBuilder::MeshBuilder(std::shared_ptr<VoxelWorld> world) {
	MeshBuilder::world = world;
	MeshBuilder::worldWidth = world->getWidth();
	MeshBuilder::worldDepth = world->getDepth();
}

MeshBuilder::~MeshBuilder() {

}

float MeshBuilder::fSample(std::shared_ptr<Chunk> chunk, int x, int y, int z) {
	double s = 1.0;

	if (x < 0 || x >= CHUNK_SIZE_X || z < 0 || z >= CHUNK_SIZE_Z) {
		Vector3 position = chunk->getOffset() + Vector3(x, y, z);
		
		if (position.x < 0 || position.x >= worldWidth * CHUNK_SIZE_X
			|| position.z < 0 || position.z >= worldDepth * CHUNK_SIZE_Z)
			return s;

		auto neighbour = world->getChunk(position);
		if (neighbour) {
			s = neighbour->getVoxel(
				(x + CHUNK_SIZE_X) % CHUNK_SIZE_X,
				(y + CHUNK_SIZE_Y) % CHUNK_SIZE_Y,
				(z + CHUNK_SIZE_Z) % CHUNK_SIZE_Z);
		}
		else {
			s = chunk->isVoxel(x, y, z);
		}
	}
	else {
		s = chunk->getVoxel(
			x % CHUNK_SIZE_X,
			y % CHUNK_SIZE_Y,
			z % CHUNK_SIZE_Z);
	}

	return s;
}

void MeshBuilder::buildCell(int x, int y, int z, std::shared_ptr<Chunk> chunk, float** vertices, int** faces, int* counts) {
	Vector3 offset = chunk->getOffset();
	int gridSize = chunk->getGridSize();
	int e[2];
	int v[3];
	float p[3];
	float grid[8];
	int edges[12];
	int cube_index = 0;
	int volume_index = fn::fi3(x, y, z);

	for (int i = 0; i < 8; ++i) {
		v[0] = cubeVerts[i][0] * gridSize + x;
		v[1] = cubeVerts[i][1] * gridSize + y;
		v[2] = cubeVerts[i][2] * gridSize + z;

		float s = fSample(chunk, v[0] - 1, v[1], v[2] - 1);

		grid[i] = s;
		cube_index |= (s > 0) ? 1 << i : 0;
	}

	int edge_mask = edgeTable[cube_index];
	if (edge_mask == 0 || edge_mask == 0xFF)
		return;

	for (int i = 0; i < 12; ++i) {
		if ((edge_mask & (1 << i)) == 0) {
			edges[i] = -1;
			continue;
		}

		edges[i] = counts[0];

		e[0] = edgeIndex[i][0];
		e[1] = edgeIndex[i][1];

		p[0] = cubeVerts[e[0]][0] * gridSize;
		p[1] = cubeVerts[e[0]][1] * gridSize;
		p[2] = cubeVerts[e[0]][2] * gridSize;

		float a = grid[e[0]];
		float b = grid[e[1]];
		float d = a - b;
		float t = 0;

		if (abs(d) > 1e-6)
			t = a / d;

		vertices[counts[0]][0] = (offset.x + x + p[0] + t * edgeDirection[i][0] * gridSize) - 0.5;
		vertices[counts[0]][1] = (offset.y + y + p[1] + t * edgeDirection[i][1] * gridSize) - 0.5;
		vertices[counts[0]][2] = (offset.z + z + p[2] + t * edgeDirection[i][2] * gridSize) - 0.5;
		counts[0]++;
	}

	for (int i = 0; triTable[cube_index][i] != -1; i += 3) {
		faces[counts[1]][0] = edges[triTable[cube_index][i]];
		faces[counts[1]][1] = edges[triTable[cube_index][i + 1]];
		faces[counts[1]][2] = edges[triTable[cube_index][i + 2]];
		faces[counts[1]][3] = volume_index;
		counts[1]++;
	}
}

int* MeshBuilder::buildVertices(std::shared_ptr<Chunk> chunk, float** vertices, int** faces) {
	Vector3 o = chunk->getOffset();
	int gridSize = chunk->getGridSize();
	int counts[2] = { 0, 0 };
	int x[3];

	for (x[2] = 0; x[2] < CHUNK_SIZE_Z / gridSize; ++x[2])
		for (x[1] = 0; x[1] < CHUNK_SIZE_Y / gridSize - 1; ++x[1])
			for (x[0] = 0; x[0] < CHUNK_SIZE_X / gridSize; ++x[0]) {
				if (o.x + x[0] >= CHUNK_SIZE_X * worldWidth) continue;
				if (o.z + x[2] >= CHUNK_SIZE_Z * worldDepth) continue;
				buildCell(x[0] * gridSize, x[1] * gridSize, x[2] * gridSize, chunk, vertices, faces, counts);
			}

	return counts;
}