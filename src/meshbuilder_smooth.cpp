#include "meshbuilder_smooth.h"
#include "graphnode.h"
#include "navigator.h"
#include "ThreadPool.h"
#include "ecogame.h"

using namespace godot;

MeshBuilder_Smooth::MeshBuilder_Smooth() {

}

MeshBuilder_Smooth::~MeshBuilder_Smooth() {

}

float fSample(std::shared_ptr<Chunk> chunk, int x, int y, int z) {
	double s = 1;

	if (x < 0 || x >= CHUNK_SIZE_X || y < 0 ||y >= CHUNK_SIZE_Y || z < 0 || z >= CHUNK_SIZE_Z) {
		auto neighbour = EcoGame::get()->getChunk(Vector3(x, y, z));
		if (neighbour && neighbour->getMeshInstanceId()) {
			s = neighbour->getVoxel(
				x % CHUNK_SIZE_X,
				y % CHUNK_SIZE_Y,
				z % CHUNK_SIZE_Z);
		}
		else {
			s = chunk->isVoxelF(x, y, z);
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

Array MeshBuilder_Smooth::buildVertices(std::shared_ptr<Chunk> chunk, float** vertices, int** faces) {
	Vector3 offset = chunk->getOffset();

	int vertsCount = 0, facesCount = 0;
	int DIMS[3] = {
		CHUNK_SIZE_X + 2,
		CHUNK_SIZE_Y + 2,
		CHUNK_SIZE_Z + 2
		//CHUNK_SIZE_X,
		//CHUNK_SIZE_Y,
		//CHUNK_SIZE_Z
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
					//v[0] = cubeVerts[i][0] + x[0];
					//v[1] = cubeVerts[i][1] + x[1];
					//v[2] = cubeVerts[i][2] + x[2];

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

void MeshBuilder_Smooth::createNodes(std::shared_ptr<Chunk> chunk) {
	if (chunk->isNavigatable()) return;

	int i, j, k, l, w, h, u, v, n, d, side = 0, face = -1, v0 = -1, v1 = -1, idx;
	bool backFace, b, done = false, doInitializeNodes = false;

	int DIMS[3] = {
		CHUNK_SIZE_X,
		CHUNK_SIZE_Y,
		CHUNK_SIZE_Z
	};

	vector<int> vertexOffsets;
	vector<std::shared_ptr<NodeUpdateData>> updateData;

	for (i = 0; i < vertexOffsets.size(); i++) {
		vertexOffsets[i] = 0;
	}

	int* mask = MeshBuilder_Smooth::getMaskPool().borrow();
	memset(mask, -1, BUFFER_SIZE * sizeof(*mask));

	int bl[3];
	int tl[3];
	int tr[3];
	int br[3];

	int x[3] = { 0, 0, 0 };
	int q[3] = { 0, 0, 0 };
	int r[3] = { 0, 0, 0 };
	int s[3] = { 0, 0, 0 };
	int du[3] = { 0, 0, 0 };
	int dv[3] = { 0, 0, 0 };

	for (backFace = true, b = false; b != backFace; backFace = backFace && b, b = !b) {
		// sweep over 3-axes
		for (d = 0; d < 3; ++d) {
			u = (d + 1) % 3;
			v = (d + 2) % 3;

			x[0] = 0;
			x[1] = 0;
			x[2] = 0;

			q[0] = 0;
			q[1] = 0;
			q[2] = 0;
			q[d] = 1;

			if (d == 0) {
				side = backFace ? WEST : EAST;
			}
			else if (d == 1) {
				side = backFace ? BOTTOM : TOP;
			}
			else if (d == 2) {
				side = backFace ? SOUTH : NORTH;
			}

			for (x[d] = -1; x[d] < DIMS[d]; ) {
				// compute mask
				n = 0;
				for (x[v] = 0; x[v] < DIMS[v]; x[v]++) {
					for (x[u] = 0; x[u] < DIMS[u]; x[u]++) {
						v0 = (0 <= x[d]) ? fSample(chunk, x[0], x[1], x[2]) <= 0 : -1;
						v1 = (x[d] < DIMS[d] - 1) ? fSample(chunk, x[0] + q[0], x[1] + q[1], x[2] + q[2]) <= 0 : -1;
						mask[n++] = (v0 != -1 && v0 == v1) ? -1 : backFace ? v1 : v0;
					}
				}

				// increment x[d]
				x[d]++;
				// generate mesh for mask using lexicographic ordering
				n = 0;

				for (j = 0; j < DIMS[v]; j++) {
					for (i = 0; i < DIMS[u]; ) {

						if (mask[n] > 0) {

							// compute width
							w = 1;
							while (i + w < DIMS[u] && mask[n + w] > 0 && mask[n + w] == mask[n]) w++;

							// compute height (this is slightly awkward
							done = false;
							for (h = 1; j + h < DIMS[v]; h++) {
								for (k = 0; k < w; k++) {
									face = mask[n + k + h * DIMS[u]];
									if (face <= 0 || face != mask[n]) {
										done = true;
										break;
									}
								}

								if (done) {
									break;
								}
							}

							if (mask[n] > 0) {
								// add quad
								x[u] = i;
								x[v] = j;

								du[0] = 0;
								du[1] = 0;
								du[2] = 0;
								du[u] = w;

								dv[0] = 0;
								dv[1] = 0;
								dv[2] = 0;
								dv[v] = h;

								bl[0] = x[0];
								bl[1] = x[1];
								bl[2] = x[2];

								tl[0] = x[0] + du[0];
								tl[1] = x[1] + du[1];
								tl[2] = x[2] + du[2];

								tr[0] = x[0] + du[0] + dv[0];
								tr[1] = x[1] + du[1] + dv[1];
								tr[2] = x[2] + du[2] + dv[2];

								br[0] = x[0] + dv[0];
								br[1] = x[1] + dv[1];
								br[2] = x[2] + dv[2];

								auto data = std::make_shared<NodeUpdateData>();
								data->bl[0] = bl[0]; data->bl[1] = bl[1]; data->bl[2] = bl[2];
								data->tl[0] = tl[0]; data->tl[1] = tl[1]; data->tl[2] = tl[2];
								data->tr[0] = tr[0]; data->tr[1] = tr[1]; data->tr[2] = tr[2];
								data->br[0] = br[0]; data->br[1] = br[1]; data->br[2] = br[2];
								data->side = side;
								data->type = mask[n];
								updateData.push_back(data);
							}

							// zero-out mask
							for (l = 0; l < h; ++l) {
								for (k = 0; k < w; ++k) {
									mask[n + k + l * DIMS[u]] = -1;
								}
							}

							// increment counters and continue
							i += w;
							n += w;
						}
						else {
							i++;
							n++;
						}
					}
				}
			}
		}
	}

	if (!updateData.empty())
		ThreadPool::getNav()->submitTask(boost::bind(&MeshBuilder_Smooth::updateGraph, this, chunk, updateData));

	MeshBuilder_Smooth::getMaskPool().ret(mask);
}

void MeshBuilder_Smooth::updateGraph(std::shared_ptr<Chunk> chunk, vector<std::shared_ptr<NodeUpdateData>> updateData) {
	while (!updateData.empty()) {
		auto data = updateData.back();
		updateGraphNode(chunk, data);
		updateData.pop_back();
		data.reset();
	}

	Navigator::get()->updateGraph(chunk);
}

void MeshBuilder_Smooth::updateGraphNode(std::shared_ptr<Chunk> chunk, std::shared_ptr<NodeUpdateData> data) {
	int nxi, nyi, nzi;
	float nx, ny, nz;
	Vector3 offset = chunk->getOffset();
	Vector3 otherOffset;

	switch (data->side) {
	case TOP:
		//float w = TEXTURE_SCALE * abs(bl[2] - tl[2]);
		//float h = TEXTURE_SCALE * abs(bl[0] - br[0]);
		nyi = data->bl[1] - 1;
		ny = nyi + 0.5;
		for (nzi = data->bl[2]; nzi < data->tl[2]; nzi++) {
			for (nxi = data->bl[0]; nxi < data->br[0]; nxi++) {
				nx = nxi + 0.5;
				nz = nzi + 0.5;
				otherOffset.x = nx;
				otherOffset.y = ny;
				otherOffset.z = nz;
				otherOffset = fn::toChunkCoords(offset + otherOffset);
				otherOffset *= Vector3(CHUNK_SIZE_X, 0, CHUNK_SIZE_Z);
				if (offset == otherOffset && fSample(chunk, nxi, nyi + 1, nzi) > 0) {
					auto node = GraphNavNode::_new();
					node->setPoint(offset + Vector3(nx, ny, nz));
					node->setVoxel(data->type);
					chunk->addNode(std::shared_ptr<GraphNavNode>(node), GraphNavNode::DIRECTION::TOP);
				}
			}
		}
		break;
	case BOTTOM:
		//float w = TEXTURE_SCALE * abs(bl[0] - br[0]);
		//float h = TEXTURE_SCALE * abs(bl[2] - tl[2]);
		nyi = data->bl[1] + 1;
		ny = nyi - 0.5;
		for (nzi = data->bl[2]; nzi < data->tl[2]; nzi++) {
			for (nxi = data->bl[0]; nxi < data->br[0]; nxi++) {
				nx = nxi + 0.5;
				nz = nzi + 0.5;
				otherOffset.x = nx;
				otherOffset.y = ny;
				otherOffset.z = nz;
				otherOffset = fn::toChunkCoords(offset + otherOffset);
				otherOffset *= Vector3(CHUNK_SIZE_X, 0, CHUNK_SIZE_Z);
				if (offset == otherOffset && fSample(chunk, nxi, nyi - 2, nzi) > 0) {
					auto node = GraphNavNode::_new();
					node->setPoint(offset + Vector3(nx, ny, nz));
					node->setVoxel(data->type);
					chunk->addNode(std::shared_ptr<GraphNavNode>(node), GraphNavNode::DIRECTION::BOTTOM);
				}
			}
		}
		break;
	case WEST:
		//float w = TEXTURE_SCALE * abs(bl[2] - br[2]);
		//float h = TEXTURE_SCALE * abs(bl[1] - tl[1]);
		nxi = data->bl[0] + 1;
		nx = nxi - 0.5;
		for (nzi = data->bl[2]; nzi < data->br[2]; nzi++) {
			for (nyi = data->bl[1]; nyi < data->tl[1]; nyi++) {
				ny = nyi + 0.5;
				nz = nzi + 0.5;
				otherOffset.x = nx;
				otherOffset.y = ny;
				otherOffset.z = nz;
				otherOffset = fn::toChunkCoords(offset + otherOffset);
				otherOffset *= Vector3(CHUNK_SIZE_X, 0, CHUNK_SIZE_Z);
				if (offset == otherOffset && fSample(chunk, nxi - 2, nyi, nzi) > 0) {
					auto node = GraphNavNode::_new();
					node->setPoint(offset + Vector3(nx, ny, nz));
					node->setVoxel(data->type);
					chunk->addNode(std::shared_ptr<GraphNavNode>(node), GraphNavNode::DIRECTION::WEST);
				}
			}
		}
		break;
	case EAST:
		//float w = TEXTURE_SCALE * abs(bl[1] - tl[1]);
		//float h = TEXTURE_SCALE * abs(bl[2] - br[2]);
		nxi = data->bl[0] - 1;
		nx = nxi + 0.5;
		for (nzi = data->bl[2]; nzi < data->br[2]; nzi++) {
			for (nyi = data->bl[1]; nyi < data->tl[1]; nyi++) {
				ny = nyi + 0.5;
				nz = nzi + 0.5;
				otherOffset.x = nx;
				otherOffset.y = ny;
				otherOffset.z = nz;
				otherOffset = fn::toChunkCoords(offset + otherOffset);
				otherOffset *= Vector3(CHUNK_SIZE_X, 0, CHUNK_SIZE_Z);
				if (offset == otherOffset && fSample(chunk, nxi + 1, nyi, nzi) > 0) {
					auto node = GraphNavNode::_new();
					node->setPoint(offset + Vector3(nx, ny, nz));
					node->setVoxel(data->type);
					chunk->addNode(std::shared_ptr<GraphNavNode>(node), GraphNavNode::DIRECTION::EAST);
				}
			}
		}
		break;
	case NORTH:
		//float w = TEXTURE_SCALE * abs(bl[0] - tl[0]);
		//float h = TEXTURE_SCALE * abs(bl[1] - br[1]);
		nzi = data->bl[2] - 1;
		nz = nzi + 0.5;
		for (nyi = data->bl[1]; nyi < data->br[1]; nyi++) {
			for (nxi = data->bl[0]; nxi < data->tl[0]; nxi++) {
				nx = nxi + 0.5;
				ny = nyi + 0.5;
				otherOffset.x = nx;
				otherOffset.y = ny;
				otherOffset.z = nz;
				otherOffset = fn::toChunkCoords(offset + otherOffset);
				otherOffset *= Vector3(CHUNK_SIZE_X, 0, CHUNK_SIZE_Z);
				if (offset == otherOffset && fSample(chunk, nxi, nyi, nzi + 1) > 0) {
					auto node = GraphNavNode::_new();
					node->setPoint(offset + Vector3(nx, ny, nz));
					node->setVoxel(data->type);
					chunk->addNode(std::shared_ptr<GraphNavNode>(node), GraphNavNode::DIRECTION::NORTH);
				}
			}
		}
		break;
	case SOUTH:
		//float w = TEXTURE_SCALE * abs(bl[1] - br[1]);
		//float h = TEXTURE_SCALE * abs(bl[0] - tl[0]);
		nzi = data->bl[2] + 1;
		nz = nzi - 0.5;
		for (nyi = data->bl[1]; nyi < data->br[1]; nyi++) {
			for (nxi = data->bl[0]; nxi < data->tl[0]; nxi++) {
				nx = nxi + 0.5;
				ny = nyi + 0.5;
				otherOffset.x = nx;
				otherOffset.y = ny;
				otherOffset.z = nz;
				otherOffset = fn::toChunkCoords(offset + otherOffset);
				otherOffset *= Vector3(CHUNK_SIZE_X, 0, CHUNK_SIZE_Z);
				if (offset == otherOffset && fSample(chunk, nxi, nyi, nzi - 2) > 0) {
					auto node = GraphNavNode::_new();
					node->setPoint(offset + Vector3(nx, ny, nz));
					node->setVoxel(data->type);
					chunk->addNode(std::shared_ptr<GraphNavNode>(node), GraphNavNode::DIRECTION::SOUTH);
				}
			}
		}
		break;
	}
}