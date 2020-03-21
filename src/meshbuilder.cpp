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
	double s = 1;

	if (x < 0 || x >= CHUNK_SIZE_X || y < 0 || y >= CHUNK_SIZE_Y || z < 0 || z >= CHUNK_SIZE_Z) {
		auto neighbour = EcoGame::get()->getChunk(Vector3(x, y, z));
		if (neighbour && neighbour->getMeshInstanceId()) {
			s = neighbour->getVoxel(
				x % CHUNK_SIZE_X,
				y % CHUNK_SIZE_Y,
				z % CHUNK_SIZE_Z);
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

	//s = chunk->isVoxelF(v[0], v[1], v[2]);
	return s;
}

Array MeshBuilder::buildVertices(std::shared_ptr<Chunk> chunk, float** vertices, int** faces) {
	Vector3 offset = chunk->getOffset();

	int vertsCount = 0, facesCount = 0;
	int DIMS[3] = {
		CHUNK_SIZE_X + 1,
		CHUNK_SIZE_Y + 1,
		CHUNK_SIZE_Z + 1
	};

	int x[3];
	int e[2];
	int v[3];
	float p[3];
	float grid[8];
	int edges[12];

	//March over the volume
	for (x[2] = 0; x[2] < DIMS[2] - 1; ++x[2])
		for (x[1] = 0; x[1] < DIMS[1] - 1; ++x[1])
			for (x[0] = 0; x[0] < DIMS[0] - 1; ++x[0]) {
				//For each cell, compute cube mask
				int cube_index = 0;
				
				for (int i = 0; i < 8; ++i) {
					v[0] = (cubeVerts[i][0] + x[0]) - 1;
					v[1] = (cubeVerts[i][1] + x[1]) - 1;
					v[2] = (cubeVerts[i][2] + x[2]) - 1;

					float s = fSample(chunk, v[0], v[1], v[2]);

					grid[i] = s;
					cube_index |= (s > 0) ? 1 << i : 0;
				}

				//Compute vertices
				int edge_mask = edgeTable[cube_index];
				if (edge_mask == 0 || edge_mask == 0xFF)
					continue;

				for (int i = 0; i < 12; ++i) {
					if ((edge_mask & (1 << i)) == 0) {
						edges[i] = -1;
						continue;
					}

					edges[i] = vertsCount;

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
					
					vertices[vertsCount][0] = offset.x + x[0] + p[0] + t * edgeDirection[i][0];
					vertices[vertsCount][1] = offset.y + x[1] + p[1] + t * edgeDirection[i][1];
					vertices[vertsCount][2] = offset.z + x[2] + p[2] + t * edgeDirection[i][2];
					vertsCount++;
				}

				for (int i = 0; triTable[cube_index][i] != -1; i += 3) {
					faces[facesCount][0] = edges[triTable[cube_index][i]];
					faces[facesCount][1] = edges[triTable[cube_index][i + 1]];
					faces[facesCount][2] = edges[triTable[cube_index][i + 2]];
					facesCount++;
				}
			}

	return Array::make(vertsCount, facesCount);
}