#include "meshbuilder.h"

using namespace godot;

MeshBuilder::MeshBuilder() {
	//Godot::print(String("{0} quads").format(Array::make(size)));
}

MeshBuilder::~MeshBuilder() {
	// add your cleanup here
}

vector<int> MeshBuilder::buildVertices(Chunk* chunk, vector<float*> buffers) {
	//Godot::print(String("offset: {0}, volume[0]: {1}, volume[1]: {2}").format(Array::make(offset, (*volume)[0], (*volume)[1])));

	Vector3 offset = chunk->getOffset();

	int i, j, k, l, w, h, u, v, n, d, side = 0, face = -1, v0 = -1, v1 = -1, type;
	bool backFace, b, done = false;

	vector<int> vertexOffsets;
	vertexOffsets.resize(buffers.size());

	for (i = 0; i < vertexOffsets.size(); i++) {
		vertexOffsets[i] = 0;
	}

	int* mask = MeshBuilder::getMaskPool().borrow();
	//int* mask = new int[BUFFER_SIZE];
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

			for (x[d] = -1; x[d] < dims[d]; ) {
				// compute mask
				n = 0;
				for (x[v] = 0; x[v] < dims[v]; x[v]++) {
					for (x[u] = 0; x[u] < dims[u]; x[u]++) {
						v0 = (0 <= x[d]) ? chunk->getVoxel(x[0], x[1], x[2]) : -1;
						v1 = (x[d] < dims[d] - 1) ? chunk->getVoxel(x[0] + q[0], x[1] + q[1], x[2] + q[2]) : -1;
						mask[n++] = (v0 != -1 && v0 == v1) ? -1 : backFace ? v1 : v0;
					}
				}

				// increment x[d]
				x[d]++;
				// generate mesh for mask using lexicographic ordering
				n = 0;

				for (j = 0; j < dims[v]; j++) {
					for (i = 0; i < dims[u]; ) {

						if (mask[n] > 0) {
							
							// compute width
							w = 1;
							while (i + w < dims[u] && mask[n + w] > 0 && mask[n + w] == mask[n]) w++;

							// compute height (this is slightly awkward
							done = false;
							for (h = 1; j + h < dims[v]; h++) {
								for (k = 0; k < w; k++) {
									face = mask[n + k + h * dims[u]];
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
								type = mask[n];
								vertexOffsets[type] = quad(offset, bl, tl, tr, br, buffers[type - 1], side, type, vertexOffsets[type]);
							}

							// zero-out mask
							for (l = 0; l < h; ++l) {
								for (k = 0; k < w; ++k) {
									mask[n + k + l * dims[u]] = -1;
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

	MeshBuilder::getMaskPool().ret(mask);
	//delete[] mask;
	return vertexOffsets;
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