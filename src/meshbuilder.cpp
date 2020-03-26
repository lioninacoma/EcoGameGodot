#include "meshbuilder.h"
#include "graphnode.h"
#include "navigator.h"
#include "ThreadPool.h"
#include "ecogame.h"

using namespace godot;

MeshBuilder::MeshBuilder() {

}

MeshBuilder::~MeshBuilder() {

}

float fSample(std::shared_ptr<Chunk> chunk, int x, int y, int z) {
	double s = 0;

	if (x < 0 || x >= CHUNK_SIZE_X || z < 0 || z >= CHUNK_SIZE_Z) {
		auto neighbour = EcoGame::get()->getChunk(chunk->getOffset() + Vector3(x, y, z));
		if (neighbour && neighbour->getMeshInstanceId()) {
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
	int e[2];
	int v[3];
	float p[3];
	float grid[8];
	int edges[12];
	int cube_index = 0;

	for (int i = 0; i < 8; ++i) {
		v[0] = (cubeVerts[i][0] + x) - 1;
		v[1] = (cubeVerts[i][1] + y);
		v[2] = (cubeVerts[i][2] + z) - 1;

		float s = fSample(chunk, v[0], v[1], v[2]);

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

		p[0] = cubeVerts[e[0]][0];
		p[1] = cubeVerts[e[0]][1];
		p[2] = cubeVerts[e[0]][2];

		float a = grid[e[0]];
		float b = grid[e[1]];
		float d = a - b;
		float t = 0;

		if (abs(d) > 1e-6)
			t = a / d;

		vertices[counts[0]][0] = (offset.x + x + p[0] + t * edgeDirection[i][0]) - 0.5;
		vertices[counts[0]][1] = (offset.y + y + p[1] + t * edgeDirection[i][1]) - 0.5;
		vertices[counts[0]][2] = (offset.z + z + p[2] + t * edgeDirection[i][2]) - 0.5;
		counts[0]++;
	}

	for (int i = 0; triTable[cube_index][i] != -1; i += 3) {
		faces[counts[1]][0] = edges[triTable[cube_index][i]];
		faces[counts[1]][1] = edges[triTable[cube_index][i + 1]];
		faces[counts[1]][2] = edges[triTable[cube_index][i + 2]];
		faces[counts[1]][3] = fn::fi3(x, y, z);
		counts[1]++;
	}
}

int* MeshBuilder::buildVertices(std::shared_ptr<Chunk> chunk, float** vertices, int** faces) {
	int DIMS[3] = {
		CHUNK_SIZE_X + 1,
		CHUNK_SIZE_Y,
		CHUNK_SIZE_Z + 1
	};
	int x[3];
	int counts[2] = { 0, 0 };
	Vector3 o = chunk->getOffset();

	for (x[2] = 0; x[2] < DIMS[2] - 1; ++x[2])
		for (x[1] = 0; x[1] < DIMS[1] - 1; ++x[1])
			for (x[0] = 0; x[0] < DIMS[0] - 1; ++x[0]) {
				if (o.x + x[0] >= CHUNK_SIZE_X * WORLD_SIZE) continue;
				if (o.z + x[2] >= CHUNK_SIZE_Z * WORLD_SIZE) continue;
				buildCell(x[0], x[1], x[2], chunk, vertices, faces, counts);
			}

	return counts;
}