#include "meshbuilder_smooth.h"
#include "graphnode.h"
#include "navigator.h"
#include "threadpool.h"
#include "ecogame.h"

using namespace godot;

MeshBuilder_Smooth::MeshBuilder_Smooth() {

}

MeshBuilder_Smooth::~MeshBuilder_Smooth() {

}

Array MeshBuilder_Smooth::buildVertices(std::shared_ptr<Chunk> chunk, float** vertices, int** faces) {
	Vector3 offset = chunk->getOffset();

	int vertsCount = 0, facesCount = 0;
	int DIMS[3] = {
		CHUNK_SIZE_X + 2,
		CHUNK_SIZE_Y + 2,
		CHUNK_SIZE_Z + 2
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
					v[0] = cubeVerts[i][0] + x[0] - 1;
					v[1] = cubeVerts[i][1] + x[1] - 1;
					v[2] = cubeVerts[i][2] + x[2] - 1;

					double s = 0;

					if (v[0] < 0 || v[0] >= CHUNK_SIZE_X || v[1] < 0 || v[1] >= CHUNK_SIZE_Y || v[2] < 0 || v[2] >= CHUNK_SIZE_Z) {
						auto neighbour = EcoGame::get()->getChunk(Vector3(v[0], v[1], v[2]));
						if (!neighbour) {
							s = neighbour->getVoxel(
								v[0] % CHUNK_SIZE_X,
								v[1] % CHUNK_SIZE_Y,
								v[2] % CHUNK_SIZE_Z);
						}
						else {
							s = (int)chunk->isVoxel(v[0], v[1], v[2]);
						}
					}
					else {
						s = chunk->getVoxel(
							v[0] % CHUNK_SIZE_X,
							v[1] % CHUNK_SIZE_Y,
							v[2] % CHUNK_SIZE_Z);
					}

					grid[i] = s;
					cube_index |= (grid[i] > 0) ? 1 << i : 0;
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