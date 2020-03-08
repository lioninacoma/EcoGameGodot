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
				if (offset == otherOffset && !chunk->isVoxel(nxi, nyi - 2, nzi)) {
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
				if (offset == otherOffset && !chunk->isVoxel(nxi - 2, nyi, nzi)) {
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
				if (offset == otherOffset && !chunk->isVoxel(nxi + 1, nyi, nzi)) {
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
				if (offset == otherOffset && !chunk->isVoxel(nxi, nyi, nzi + 1)) {
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
				if (offset == otherOffset && !chunk->isVoxel(nxi, nyi, nzi - 2)) {
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

vector<int> MeshBuilder::buildVerticesSmooth(std::shared_ptr<VoxelData> volume, std::shared_ptr<Chunk> chunk, Vector3 offset, const int DIMS[3], float** buffers, int buffersLen) {
	//int location[3] = { 0, 0, 0 };

	//int R[] = {
	//	// x
	//	1,
	//	// y * width
	//	DIMS[0] + 1,
	//	// z * width * height
	//	(DIMS[0] + 1) * (DIMS[1] + 1)
	//};
	//// grid cell
	//double grid[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

	//// TODO: is is the only mystery that is left
	//int buf_no = 1;

	//int n = 0;

	//// March over the voxel grid
	//for (location[2] = 0; location[2] < DIMS[2] - 1; ++location[2], n += DIMS[0], buf_no ^= 1 /*even or odd*/, R[2] = -R[2]) {

	//	// m is the pointer into the buffer we are going to use.  
	//	// This is slightly obtuse because javascript does not
	//	// have good support for packed data structures, so we must
	//	// use typed arrays :(
	//	// The contents of the buffer will be the indices of the
	//	// vertices on the previous x/y slice of the volume
	//	int m = 1 + (DIMS[0] + 1) * (1 + buf_no * (DIMS[1] + 1));

	//	for (location[1] = 0; location[1] < DIMS[1] - 1; ++location[1], ++n, m += 2) {
	//		for (location[0] = 0; location[0] < DIMS[0] - 1; ++location[0], ++n, ++m) {

	//			// Read in 8 field values around this vertex
	//			// and store them in an array
	//			// Also calculate 8-bit mask, like in marching cubes,
	//			// so we can speed up sign checks later
	//			int mask = 0, g = 0, idx = n;
	//			for (int k = 0; k < 2; ++k, idx += DIMS[0] * (DIMS[1] - 2)) {
	//				for (int j = 0; j < 2; ++j, idx += DIMS[0] - 2) {
	//					for (int i = 0; i < 2; ++i, ++g, ++idx) {
	//						double p = volume->get(i, j, k);
	//						grid[g] = p;
	//						mask |= (p < 0) ? (1 << g) : 0;
	//					}
	//				}
	//			}

	//			// Check for early termination
	//			// if cell does not intersect boundary
	//			if (mask == 0 || mask == 0xff) {
	//				continue;
	//			}

	//			// Sum up edge intersections
	//			int edge_mask = edge_table[mask];
	//			double[] v = { 0.0, 0.0, 0.0 };
	//			int e_count = 0;

	//			// For every edge of the cube...
	//			for (int i = 0; i < 12; ++i) {

	//				// Use edge mask to check if it is crossed
	//				if (!bool((edge_mask & (1 << i)))) {
	//					continue;
	//				}

	//				// If it did, increment number of edge crossings
	//				++e_count;

	//				// Now find the point of intersection
	//				int firstEdgeIndex = i << 1;
	//				int secondEdgeIndex = firstEdgeIndex + 1;
	//				// Unpack vertices
	//				int e0 = cube_edges[firstEdgeIndex];
	//				int e1 = cube_edges[secondEdgeIndex];
	//				// Unpack grid values
	//				double g0 = grid[e0];
	//				double g1 = grid[e1];

	//				// Compute point of intersection (linear interpolation)
	//				double t = g0 - g1;
	//				if (Math.abs(t) > 1e-6) {
	//					t = g0 / t;
	//				}
	//				else {
	//					continue;
	//				}

	//				// Interpolate vertices and add up intersections
	//				// (this can be done without multiplying)
	//				for (int j = 0; j < 3; j++) {
	//					int k = 1 << j; // (1,2,4)
	//					int a = e0 & k;
	//					int b = e1 & k;
	//					if (a != b) {
	//						v[j] += bool(a) ? 1.0 - t : t;
	//					}
	//					else {
	//						v[j] += bool(a) ? 1.0 : 0;
	//					}
	//				}
	//			}

	//			// Now we just average the edge intersections
	//			// and add them to coordinate
	//			double s = 1.0 / e_count;
	//			for (int i = 0; i < 3; ++i) {
	//				v[i] = location[i] + s * v[i];
	//			}

	//			// Add vertex to buffer, store pointer to
	//			// vertex index in buffer
	//			vertexBuffer[m] = vertices.size();
	//			vertices.add(v);

	//			// Now we need to add faces together, to do this we just
	//			// loop over 3 basis components
	//			for (int i = 0; i < 3; ++i) {

	//				// The first three entries of the edge_mask
	//				// count the crossings along the edge
	//				if (!bool(edge_mask & (1 << i))) {
	//					continue;
	//				}

	//				// i = axes we are point along.
	//				// iu, iv = orthogonal axes
	//				int iu = (i + 1) % 3;
	//				int iv = (i + 2) % 3;

	//				// If we are on a boundary, skip it
	//				if (location[iu] == 0 || location[iv] == 0) {
	//					continue;
	//				}

	//				// Otherwise, look up adjacent edges in buffer
	//				int du = R[iu];
	//				int dv = R[iv];

	//				// finally, the indices for the 4 vertices
	//				// that define the face
	//				final int indexM = vertexBuffer[m];
	//				final int indexMMinusDU = vertexBuffer[m - du];
	//				final int indexMMinusDV = vertexBuffer[m - dv];
	//				final int indexMMinusDUMinusDV = vertexBuffer[m - du - dv];

	//				// Remember to flip orientation depending on the sign
	//				// of the corner.
	//				if (bool(mask & 1)) {
	//					faces.add(new int[] {
	//						indexM,
	//							indexMMinusDU,
	//							indexMMinusDUMinusDV,
	//							indexMMinusDV
	//						});
	//				}
	//				else {
	//					faces.add(new int[] {
	//						indexM,
	//							indexMMinusDV,
	//							indexMMinusDUMinusDV,
	//							indexMMinusDU
	//						});
	//				}
	//			}
	//		} // end x
	//	} // end y
	//} // end z

	////All done!  Return the result
	//return new Mesh(vertices, faces);
}