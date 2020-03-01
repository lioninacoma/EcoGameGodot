#include "meshbuilder.h"
#include "graphnode.h"
#include "navigator.h"
#include "threadpool.h"

using namespace godot;

MeshBuilder::MeshBuilder() {
	//Godot::print(String("{0} quads").format(Array::make(size)));
}

MeshBuilder::~MeshBuilder() {
	// add your cleanup here
}

vector<int> MeshBuilder::buildVertices(VoxelAsset* asset, Vector3 offset, float** buffers, int buffersLen) {
	const int DIMS[3] = { asset->getWidth(), asset->getHeight(), asset->getDepth() };
	return MeshBuilder::buildVertices(asset->getVolume(), NULL, offset, DIMS, buffers, buffersLen);
}

vector<int> MeshBuilder::buildVertices(VoxelAssetType type, Vector3 offset, float** buffers, int buffersLen) {
	VoxelAsset* asset = VoxelAssetManager::get()->getVoxelAsset(type);
	const int DIMS[3] = { asset->getWidth(), asset->getHeight(), asset->getDepth() };
	return MeshBuilder::buildVertices(VoxelAssetManager::get()->getVolume(type), NULL, offset, DIMS, buffers, buffersLen);
}

vector<int> MeshBuilder::buildVertices(std::shared_ptr<Chunk> chunk, float** buffers, int buffersLen) {
	const int DIMS[3] = { CHUNK_SIZE_X, CHUNK_SIZE_Y, CHUNK_SIZE_Z };
	return MeshBuilder::buildVertices(chunk->getVolume(), chunk, chunk->getOffset(), DIMS, buffers, buffersLen);
}

vector<int> MeshBuilder::buildVertices(std::shared_ptr<VoxelData> volume, std::shared_ptr<Chunk> chunk, Vector3 offset, const int DIMS[3], float** buffers, int buffersLen) {
	bpt::ptime start, stop;
	bpt::time_duration dur;
	long ms = 0;

	start = bpt::microsec_clock::local_time();

	//if (chunk) Godot::print(String("building mesh at {0} ...").format(Array::make(chunk->getOffset())));

	int i, j, k, l, w, h, u, v, n, d, side = 0, face = -1, v0 = -1, v1 = -1, idx;
	bool backFace, b, done = false, doInitializeNodes = false;

	if (chunk) doInitializeNodes = !chunk->isNavigatable();

	vector<int> vertexOffsets;
	vector<std::shared_ptr<NodeUpdateData>> updateData;
	vertexOffsets.resize(buffersLen);

	for (i = 0; i < vertexOffsets.size(); i++) {
		vertexOffsets[i] = 0;
	}

	int* mask = MeshBuilder::getMaskPool().borrow();
	memset(mask, -1, BUFFER_SIZE * sizeof(*mask));

	int bl[3];
	int tl[3];
	int tr[3];
	int br[3];

	int x[3] = { 0, 0, 0 };
	int q[3] = { 0, 0, 0 };
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
						v0 = (0 <= x[d]) ? volume->get(x[0], x[1], x[2]) : -1;
						v1 = (x[d] < DIMS[d] - 1) ? volume->get(x[0] + q[0], x[1] + q[1], x[2] + q[2]) : -1;
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

								// create quad
								idx = mask[n] - 1;
								vertexOffsets[idx] = quad(offset, bl, tl, tr, br, buffers[idx], side, mask[n], vertexOffsets[idx]);

								if (chunk && doInitializeNodes) {
									auto data = std::make_shared<NodeUpdateData>();
									data->bl[0] = bl[0]; data->bl[1] = bl[1]; data->bl[2] = bl[2];
									data->tl[0] = tl[0]; data->tl[1] = tl[1]; data->tl[2] = tl[2];
									data->tr[0] = tr[0]; data->tr[1] = tr[1]; data->tr[2] = tr[2];
									data->br[0] = br[0]; data->br[1] = br[1]; data->br[2] = br[2];
									data->side = side;
									data->type = mask[n];
									updateData.push_back(data);
								}
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
		ThreadPool::getNav()->submitTask(boost::bind(&MeshBuilder::updateGraph, this, chunk, updateData));

	stop = bpt::microsec_clock::local_time();
	dur = stop - start;
	ms = dur.total_milliseconds();

	//if (chunk) Godot::print(String("mesh at {0} built in {1} ms").format(Array::make(chunk->getOffset(), ms)));
	//if (!chunk) Godot::print(String("mesh built").format(Array::make()));

	MeshBuilder::getMaskPool().ret(mask);
	return vertexOffsets;
}

void MeshBuilder::updateGraph(std::shared_ptr<Chunk> chunk, vector<std::shared_ptr<NodeUpdateData>> updateData) {
	while (!updateData.empty()) {
		auto data = updateData.back();
		updateGraphNode(chunk, data);
		updateData.pop_back();
		data.reset();
	}

	Navigator::get()->updateGraph(chunk);
}

void MeshBuilder::updateGraphNode(std::shared_ptr<Chunk> chunk, std::shared_ptr<NodeUpdateData> data) {
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
				if (offset == otherOffset && !chunk->isVoxel(nxi, nyi + 1, nzi)) {
					auto node = GraphNavNode::_new();
					node->setPoint(offset + Vector3(nx, ny, nz));
					node->setVoxel(data->type);
					chunk->addNode(std::shared_ptr<GraphNavNode>(node));
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
				if (offset == otherOffset && !chunk->isVoxel(nxi, nyi - 2, nzi)) {
					auto node = GraphNavNode::_new();
					node->setPoint(offset + Vector3(nx, ny, nz));
					node->setVoxel(data->type);
					chunk->addNode(std::shared_ptr<GraphNavNode>(node));
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
				if (offset == otherOffset && !chunk->isVoxel(nxi - 2, nyi, nzi)) {
					auto node = GraphNavNode::_new();
					node->setPoint(offset + Vector3(nx, ny, nz));
					node->setVoxel(data->type);
					chunk->addNode(std::shared_ptr<GraphNavNode>(node));
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
				if (offset == otherOffset && !chunk->isVoxel(nxi + 1, nyi, nzi)) {
					auto node = GraphNavNode::_new();
					node->setPoint(offset + Vector3(nx, ny, nz));
					node->setVoxel(data->type);
					chunk->addNode(std::shared_ptr<GraphNavNode>(node));
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
				if (offset == otherOffset && !chunk->isVoxel(nxi, nyi, nzi + 1)) {
					auto node = GraphNavNode::_new();
					node->setPoint(offset + Vector3(nx, ny, nz));
					node->setVoxel(data->type);
					chunk->addNode(std::shared_ptr<GraphNavNode>(node));
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
				if (offset == otherOffset && !chunk->isVoxel(nxi, nyi, nzi - 2)) {
					auto node = GraphNavNode::_new();
					node->setPoint(offset + Vector3(nx, ny, nz));
					node->setVoxel(data->type);
					chunk->addNode(std::shared_ptr<GraphNavNode>(node));
				}
			}
		}
		break;
	}
}

float MeshBuilder::getU(int type) {
	return (float)floor(type % TEXTURE_ATLAS_LEN) / (float)TEXTURE_ATLAS_LEN;
}

float MeshBuilder::getV(int type) {
	return (float)floor(type / TEXTURE_ATLAS_LEN) / (float)TEXTURE_ATLAS_LEN;
}

int MeshBuilder::quad(Vector3 offset, int bl[], int tl[], int tr[], int br[], float* vertices, int side, int type, int vertexOffset) {
	if (side == TOP) {
		vertexOffset = createTop(offset, bl, tl, tr, br, vertices, type, vertexOffset);
	}
	else if (side == BOTTOM) {
		vertexOffset = createBottom(offset, bl, tl, tr, br, vertices, type, vertexOffset);
	}
	else if (side == WEST) {
		vertexOffset = createLeft(offset, bl, tl, tr, br, vertices, type, vertexOffset);
	}
	else if (side == EAST) {
		vertexOffset = createRight(offset, bl, tl, tr, br, vertices, type, vertexOffset);
	}
	else if (side == SOUTH) {
		vertexOffset = createBack(offset, bl, tl, tr, br, vertices, type, vertexOffset);
	}
	else if (side == NORTH) {
		vertexOffset = createFront(offset, bl, tl, tr, br, vertices, type, vertexOffset);
	}

	return vertexOffset;
}

int MeshBuilder::createTop(Vector3 offset, int bl[], int tl[], int tr[], int br[], float* vertices, int type, int vertexOffset) {
	float w = TEXTURE_SCALE * abs(bl[2] - tl[2]);
	float h = TEXTURE_SCALE * abs(bl[0] - br[0]);

	float u = getU(type);
	float v = getV(type);

	float u2 = u + w;
	float v2 = v + h;

	vertices[vertexOffset++] = offset.x + bl[0];
	vertices[vertexOffset++] = offset.y + bl[1];
	vertices[vertexOffset++] = offset.z + bl[2];
	vertices[vertexOffset++] = 0;
	vertices[vertexOffset++] = 1;
	vertices[vertexOffset++] = 0;
	vertices[vertexOffset++] = 0;
	vertices[vertexOffset++] = h;

	vertices[vertexOffset++] = offset.x + tl[0];
	vertices[vertexOffset++] = offset.y + tl[1];
	vertices[vertexOffset++] = offset.z + tl[2];
	vertices[vertexOffset++] = 0;
	vertices[vertexOffset++] = 1;
	vertices[vertexOffset++] = 0;
	vertices[vertexOffset++] = w;
	vertices[vertexOffset++] = h;

	vertices[vertexOffset++] = offset.x + tr[0];
	vertices[vertexOffset++] = offset.y + tr[1];
	vertices[vertexOffset++] = offset.z + tr[2];
	vertices[vertexOffset++] = 0;
	vertices[vertexOffset++] = 1;
	vertices[vertexOffset++] = 0;
	vertices[vertexOffset++] = w;
	vertices[vertexOffset++] = 0;

	vertices[vertexOffset++] = offset.x + br[0];
	vertices[vertexOffset++] = offset.y + br[1];
	vertices[vertexOffset++] = offset.z + br[2];
	vertices[vertexOffset++] = 0;
	vertices[vertexOffset++] = 1;
	vertices[vertexOffset++] = 0;
	vertices[vertexOffset++] = 0;
	vertices[vertexOffset++] = 0;

	return vertexOffset;
}

int MeshBuilder::createBottom(Vector3 offset, int bl[], int tl[], int tr[], int br[], float* vertices, int type, int vertexOffset) {
	float w = TEXTURE_SCALE * abs(bl[0] - br[0]);
	float h = TEXTURE_SCALE * abs(bl[2] - tl[2]);

	float u = getU(type);
	float v = getV(type);
	float u2 = u + w;
	float v2 = v + h;

	vertices[vertexOffset++] = offset.x + bl[0];
	vertices[vertexOffset++] = offset.y + bl[1];
	vertices[vertexOffset++] = offset.z + bl[2];
	vertices[vertexOffset++] = 0;
	vertices[vertexOffset++] = -1;
	vertices[vertexOffset++] = 0;
	vertices[vertexOffset++] = 0;
	vertices[vertexOffset++] = h;

	vertices[vertexOffset++] = offset.x + br[0];
	vertices[vertexOffset++] = offset.y + br[1];
	vertices[vertexOffset++] = offset.z + br[2];
	vertices[vertexOffset++] = 0;
	vertices[vertexOffset++] = -1;
	vertices[vertexOffset++] = 0;
	vertices[vertexOffset++] = w;
	vertices[vertexOffset++] = h;

	vertices[vertexOffset++] = offset.x + tr[0];
	vertices[vertexOffset++] = offset.y + tr[1];
	vertices[vertexOffset++] = offset.z + tr[2];
	vertices[vertexOffset++] = 0;
	vertices[vertexOffset++] = -1;
	vertices[vertexOffset++] = 0;
	vertices[vertexOffset++] = w;
	vertices[vertexOffset++] = 0;

	vertices[vertexOffset++] = offset.x + tl[0];
	vertices[vertexOffset++] = offset.y + tl[1];
	vertices[vertexOffset++] = offset.z + tl[2];
	vertices[vertexOffset++] = 0;
	vertices[vertexOffset++] = -1;
	vertices[vertexOffset++] = 0;
	vertices[vertexOffset++] = 0;
	vertices[vertexOffset++] = 0;

	return vertexOffset;
}

int MeshBuilder::createLeft(Vector3 offset, int bl[], int tl[], int tr[], int br[], float* vertices, int type, int vertexOffset) {
	float w = TEXTURE_SCALE * abs(bl[2] - br[2]);
	float h = TEXTURE_SCALE * abs(bl[1] - tl[1]);

	float u = getU(type);
	float v = getV(type);
	float u2 = u + w;
	float v2 = v + h;

	vertices[vertexOffset++] = offset.x + bl[0];
	vertices[vertexOffset++] = offset.y + bl[1];
	vertices[vertexOffset++] = offset.z + bl[2];
	vertices[vertexOffset++] = -1;
	vertices[vertexOffset++] = 0;
	vertices[vertexOffset++] = 0;
	vertices[vertexOffset++] = 0;
	vertices[vertexOffset++] = h;

	vertices[vertexOffset++] = offset.x + br[0];
	vertices[vertexOffset++] = offset.y + br[1];
	vertices[vertexOffset++] = offset.z + br[2];
	vertices[vertexOffset++] = -1;
	vertices[vertexOffset++] = 0;
	vertices[vertexOffset++] = 0;
	vertices[vertexOffset++] = w;
	vertices[vertexOffset++] = h;

	vertices[vertexOffset++] = offset.x + tr[0];
	vertices[vertexOffset++] = offset.y + tr[1];
	vertices[vertexOffset++] = offset.z + tr[2];
	vertices[vertexOffset++] = -1;
	vertices[vertexOffset++] = 0;
	vertices[vertexOffset++] = 0;
	vertices[vertexOffset++] = w;
	vertices[vertexOffset++] = 0;

	vertices[vertexOffset++] = offset.x + tl[0];
	vertices[vertexOffset++] = offset.y + tl[1];
	vertices[vertexOffset++] = offset.z + tl[2];
	vertices[vertexOffset++] = -1;
	vertices[vertexOffset++] = 0;
	vertices[vertexOffset++] = 0;
	vertices[vertexOffset++] = 0;
	vertices[vertexOffset++] = 0;

	return vertexOffset;
}

int MeshBuilder::createRight(Vector3 offset, int bl[], int tl[], int tr[], int br[], float* vertices, int type, int vertexOffset) {
	float w = TEXTURE_SCALE * abs(bl[1] - tl[1]);
	float h = TEXTURE_SCALE * abs(bl[2] - br[2]);

	float u = getU(type);
	float v = getV(type);
	float u2 = u + w;
	float v2 = v + h;

	vertices[vertexOffset++] = offset.x + bl[0];
	vertices[vertexOffset++] = offset.y + bl[1];
	vertices[vertexOffset++] = offset.z + bl[2];
	vertices[vertexOffset++] = 1;
	vertices[vertexOffset++] = 0;
	vertices[vertexOffset++] = 0;
	vertices[vertexOffset++] = 0;
	vertices[vertexOffset++] = h;

	vertices[vertexOffset++] = offset.x + tl[0];
	vertices[vertexOffset++] = offset.y + tl[1];
	vertices[vertexOffset++] = offset.z + tl[2];
	vertices[vertexOffset++] = 1;
	vertices[vertexOffset++] = 0;
	vertices[vertexOffset++] = 0;
	vertices[vertexOffset++] = w;
	vertices[vertexOffset++] = h;

	vertices[vertexOffset++] = offset.x + tr[0];
	vertices[vertexOffset++] = offset.y + tr[1];
	vertices[vertexOffset++] = offset.z + tr[2];
	vertices[vertexOffset++] = 1;
	vertices[vertexOffset++] = 0;
	vertices[vertexOffset++] = 0;
	vertices[vertexOffset++] = w;
	vertices[vertexOffset++] = 0;

	vertices[vertexOffset++] = offset.x + br[0];
	vertices[vertexOffset++] = offset.y + br[1];
	vertices[vertexOffset++] = offset.z + br[2];
	vertices[vertexOffset++] = 1;
	vertices[vertexOffset++] = 0;
	vertices[vertexOffset++] = 0;
	vertices[vertexOffset++] = 0;
	vertices[vertexOffset++] = 0;

	return vertexOffset;
}

int MeshBuilder::createFront(Vector3 offset, int bl[], int tl[], int tr[], int br[], float* vertices, int type, int vertexOffset) {
	float w = TEXTURE_SCALE * abs(bl[0] - tl[0]);
	float h = TEXTURE_SCALE * abs(bl[1] - br[1]);

	float u = getU(type);
	float v = getV(type);
	float u2 = u + w;
	float v2 = v + h;

	vertices[vertexOffset++] = offset.x + bl[0];
	vertices[vertexOffset++] = offset.y + bl[1];
	vertices[vertexOffset++] = offset.z + bl[2];
	vertices[vertexOffset++] = 0;
	vertices[vertexOffset++] = 0;
	vertices[vertexOffset++] = 1;
	vertices[vertexOffset++] = 0;
	vertices[vertexOffset++] = h;

	vertices[vertexOffset++] = offset.x + tl[0];
	vertices[vertexOffset++] = offset.y + tl[1];
	vertices[vertexOffset++] = offset.z + tl[2];
	vertices[vertexOffset++] = 0;
	vertices[vertexOffset++] = 0;
	vertices[vertexOffset++] = 1;
	vertices[vertexOffset++] = w;
	vertices[vertexOffset++] = h;

	vertices[vertexOffset++] = offset.x + tr[0];
	vertices[vertexOffset++] = offset.y + tr[1];
	vertices[vertexOffset++] = offset.z + tr[2];
	vertices[vertexOffset++] = 0;
	vertices[vertexOffset++] = 0;
	vertices[vertexOffset++] = 1;
	vertices[vertexOffset++] = w;
	vertices[vertexOffset++] = 0;

	vertices[vertexOffset++] = offset.x + br[0];
	vertices[vertexOffset++] = offset.y + br[1];
	vertices[vertexOffset++] = offset.z + br[2];
	vertices[vertexOffset++] = 0;
	vertices[vertexOffset++] = 0;
	vertices[vertexOffset++] = 1;
	vertices[vertexOffset++] = 0;
	vertices[vertexOffset++] = 0;

	return vertexOffset;
}

int MeshBuilder::createBack(Vector3 offset, int bl[], int tl[], int tr[], int br[], float* vertices, int type, int vertexOffset) {
	float w = TEXTURE_SCALE * abs(bl[1] - br[1]);
	float h = TEXTURE_SCALE * abs(bl[0] - tl[0]);

	float u = getU(type);
	float v = getV(type);
	float u2 = u + w;
	float v2 = v + h;

	vertices[vertexOffset++] = offset.x + bl[0];
	vertices[vertexOffset++] = offset.y + bl[1];
	vertices[vertexOffset++] = offset.z + bl[2];
	vertices[vertexOffset++] = 0;
	vertices[vertexOffset++] = 0;
	vertices[vertexOffset++] = -1;
	vertices[vertexOffset++] = 0;
	vertices[vertexOffset++] = h;

	vertices[vertexOffset++] = offset.x + br[0];
	vertices[vertexOffset++] = offset.y + br[1];
	vertices[vertexOffset++] = offset.z + br[2];
	vertices[vertexOffset++] = 0;
	vertices[vertexOffset++] = 0;
	vertices[vertexOffset++] = -1;
	vertices[vertexOffset++] = w;
	vertices[vertexOffset++] = h;

	vertices[vertexOffset++] = offset.x + tr[0];
	vertices[vertexOffset++] = offset.y + tr[1];
	vertices[vertexOffset++] = offset.z + tr[2];
	vertices[vertexOffset++] = 0;
	vertices[vertexOffset++] = 0;
	vertices[vertexOffset++] = -1;
	vertices[vertexOffset++] = w;
	vertices[vertexOffset++] = 0;

	vertices[vertexOffset++] = offset.x + tl[0];
	vertices[vertexOffset++] = offset.y + tl[1];
	vertices[vertexOffset++] = offset.z + tl[2];
	vertices[vertexOffset++] = 0;
	vertices[vertexOffset++] = 0;
	vertices[vertexOffset++] = -1;
	vertices[vertexOffset++] = 0;
	vertices[vertexOffset++] = 0;

	return vertexOffset;
}