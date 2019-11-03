#include "ecogame.h"

using namespace godot;

void EcoGame::_register_methods() {
	register_method("buildVertices", &EcoGame::buildVertices);
	register_method("buildVolume", &EcoGame::buildVolume);
}

EcoGame::EcoGame() {
	EcoGame::noise = OpenSimplexNoise::_new();
	EcoGame::noise->set_octaves(4);
	EcoGame::noise->set_period(60.0);
	EcoGame::noise->set_persistence(0.5);

	/* Array quads = getQuads();
	size_t size = quads.size();
	String sizeStr = String("{0} quads").format(Array::make(size));
	Godot::print(sizeStr);

	for (int i = 0; i < quads.size(); i++) {
		Array quad = quads[i];
		String quadStr = String("quad{{0}, {1}, {2}}").format(
			Array::make(quad[0], quad[1], quad[2]));
		Godot::print(quadStr);
	} */
}

EcoGame::~EcoGame() {
	// add your cleanup here
}

void EcoGame::_init() {
	// initialize any variables here
}

int EcoGame::flattenIndex(int x, int y, int z) {
	return x + dims[0] * (y + dims[1] * z);
}

unsigned char EcoGame::getType(PoolByteArray volume, int x, int y, int z) {
	int index = flattenIndex(x, y, z);
	if (index >= 0 && index < BUFFER_SIZE)
		return volume[index];
	return 0;
}

int EcoGame::getVoxelNoiseY(Vector3 offset, int x, int z) {
	Vector2 noise2DV = Vector2(x + offset.x, z + offset.z) * 0.25;
	float y = noise->get_noise_2dv(noise2DV) / 2.0 + 0.5;
	y *= CHUNK_SIZE_Y;
	return (int) y;
}

float EcoGame::getVoxelNoiseChance(Vector3 offset, int x, int y, int z) {
	Vector3 noise3DV = Vector3(x + offset.x, y + offset.y, z + offset.z) * 1.5;
	return noise->get_noise_3dv(noise3DV) / 2.0 + 0.5;
}

PoolByteArray EcoGame::setVoxel(PoolByteArray volume, int x, int y, int z, char v) {
	if (x < 0 || x >= CHUNK_SIZE_X) return volume;
	if (y < 0 || y >= CHUNK_SIZE_Y) return volume;
	if (z < 0 || z >= CHUNK_SIZE_Z) return volume;
	volume.set(flattenIndex(x, y, z), v);
	return volume;
}

PoolByteArray EcoGame::buildVolume(Vector3 offset, int seed) {
	EcoGame::noise->set_seed(seed);

	PoolByteArray volume;
	volume.resize(BUFFER_SIZE);
	for (int i = 0; i < BUFFER_SIZE; i++) volume.set(i, 0);

	for (int z = 0; z < CHUNK_SIZE_Z; z++) {
		for (int x = 0; x < CHUNK_SIZE_X; x++) {
			int y = getVoxelNoiseY(offset, x, z);
			//Godot::print(String("y: {0}").format(Array::make(y)));
			for (int i = 0; i < y; i++) {
				float c = getVoxelNoiseChance(offset, x, i, z);
				//Godot::print(String("c: {0}").format(Array::make(c)));
				float t = 0.5;
				volume = setVoxel(volume, x, i, z, 1);
			}
		}
	}

	//for (int i = 0; i < BUFFER_SIZE; i++) volume.set(i, 1);
	return volume;
}

Array EcoGame::buildVertices(Vector3 offset, PoolByteArray volume) {
	Array vertices;
	int i, j, k, l, w, h, u, v, n, side = 0, vOffset = 0;

	/*String str = String("quad{{0}, {1}, {2}} 123").format(Array::make(offset[0], offset[1], offset[2]));
	Godot::print(str);*/

	int x[3];
	int q[3];
	int du[3];
	int dv[3];

	int mask[BUFFER_SIZE];
	memset(mask, -1, sizeof(mask));

	int bl[3];
	int tl[3];
	int tr[3];
	int br[3];

	for (bool backFace = true, b = false; b != backFace; backFace = backFace && b, b = !b) {
		// sweep over 3-axes
		for (int d = 0; d < 3; ++d) {
			u = (d + 1) % 3;
			v = (d + 2) % 3;

			memset(x, 0, sizeof(x));
			memset(q, 0, sizeof(q));
			q[d] = 1;

			if (d == 0) {
				side = backFace ? WEST   : EAST;
			} else if (d == 1) {
				side = backFace ? BOTTOM : TOP;
			} else if (d == 2) {
				side = backFace ? SOUTH  : NORTH;
			}

			for (x[d] = -1; x[d] < dims[d]; ) {
				// compute mask
				n = 0;
				for (x[v] = 0; x[v] < dims[v]; x[v]++) {
					for (x[u] = 0; x[u] < dims[u]; x[u]++) {
						int v0 = (0 <= x[d]) ? getType(volume, x[0], x[1], x[2]) : -1;
						int v1 = (x[d] < dims[d]-1) ? getType(volume, x[0]+q[0], x[1]+q[1], x[2]+q[2]) : -1;
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
							bool done = false;
							for (h = 1; j + h < dims[v]; h++) {
								for (k = 0; k < w; k++) {
									int face = mask[n + k + h * dims[u]];
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

								memset(du, 0, sizeof(du));
								memset(dv, 0, sizeof(dv));
								du[u] = w;
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
								quad(offset, bl, tl, tr, br, vertices, side, mask[n]);
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
						} else {
							i++;
							n++;
						}
					}
				}
			}
		}
	}
	
	return vertices;
}

float EcoGame::getU(int type) {
	return (float)floor(type % TEXTURE_ATLAS_LEN) / (float)TEXTURE_ATLAS_LEN;
}

float EcoGame::getV(int type) {
	return (float)floor(type / TEXTURE_ATLAS_LEN) / (float)TEXTURE_ATLAS_LEN;
}

void EcoGame::quad(Vector3 offset, int bl[], int tl[], int tr[], int br[], Array vertices, int side, int type) {
	if (side == TOP) {
		createTop(offset, bl, tl, tr, br, vertices, type);
	} else if (side == BOTTOM) {
		createBottom(offset, bl, tl, tr, br, vertices, type);
	} else if (side == WEST) {
		createLeft(offset, bl, tl, tr, br, vertices, type);
	} else if (side == EAST) {
		createRight(offset, bl, tl, tr, br, vertices, type);
	} else if (side == SOUTH) {
		createBack(offset, bl, tl, tr, br, vertices, type);
	} else if (side == NORTH) {
		createFront(offset, bl, tl, tr, br, vertices, type);
	}
}

void EcoGame::createTop(Vector3 offset, int bl[], int tl[], int tr[], int br[], Array vertices, int type) {
	float w = TEXTURE_SCALE * abs(bl[2] - tl[2]);
	float h = TEXTURE_SCALE * abs(bl[0] - br[0]);

	float u = getU(type);
	float v = getV(type);

	float u2 = u + w;
	float v2 = v + h;

	int offsetX = offset.x;
	int offsetY = offset.y;
	int offsetZ = offset.z;

	Array vA;
	Array vB;
	Array vC;
	Array vD;

	if (TEXTURE_MODE == 0) {
		vA = Array::make(Vector3(offsetX + bl[0], offsetY + bl[1], offsetZ + bl[2]), Vector2(u, v2));
		vB = Array::make(Vector3(offsetX + tl[0], offsetY + tl[1], offsetZ + tl[2]), Vector2(u2, v2));
		vC = Array::make(Vector3(offsetX + tr[0], offsetY + tr[1], offsetZ + tr[2]), Vector2(u2, v));
		vD = Array::make(Vector3(offsetX + br[0], offsetY + br[1], offsetZ + br[2]), Vector2(u, v));
	}
	else {
		vA = Array::make(Vector3(offsetX + bl[0], offsetY + bl[1], offsetZ + bl[2]), Vector2(0, h));
		vB = Array::make(Vector3(offsetX + tl[0], offsetY + tl[1], offsetZ + tl[2]), Vector2(w, h));
		vC = Array::make(Vector3(offsetX + tr[0], offsetY + tr[1], offsetZ + tr[2]), Vector2(w, 0));
		vD = Array::make(Vector3(offsetX + br[0], offsetY + br[1], offsetZ + br[2]), Vector2(0, 0));
	}

	vertices.append(vA);
	vertices.append(vB);
	vertices.append(vC);
	vertices.append(vD);
}

void EcoGame::createBottom(Vector3 offset, int bl[], int tl[], int tr[], int br[], Array vertices, int type) {
	float w = TEXTURE_SCALE * abs(bl[0] - br[0]);
	float h = TEXTURE_SCALE * abs(bl[2] - tl[2]);

	float u = getU(type);
	float v = getV(type);
	float u2 = u + w;
	float v2 = v + h;
	
	int offsetX = offset.x;
	int offsetY = offset.y;
	int offsetZ = offset.z;

	Array vA;
	Array vB;
	Array vC;
	Array vD;

	if (TEXTURE_MODE == 0) {
		vA = Array::make(Vector3(offsetX + bl[0], offsetY + bl[1], offsetZ + bl[2]), Vector2(u, v2));
		vB = Array::make(Vector3(offsetX + br[0], offsetY + br[1], offsetZ + br[2]), Vector2(u2, v2));
		vC = Array::make(Vector3(offsetX + tr[0], offsetY + tr[1], offsetZ + tr[2]), Vector2(u2, v));
		vD = Array::make(Vector3(offsetX + tl[0], offsetY + tl[1], offsetZ + tl[2]), Vector2(u, v));
	}
	else {
		vA = Array::make(Vector3(offsetX + bl[0], offsetY + bl[1], offsetZ + bl[2]), Vector2(0, h));
		vB = Array::make(Vector3(offsetX + br[0], offsetY + br[1], offsetZ + br[2]), Vector2(w, h));
		vC = Array::make(Vector3(offsetX + tr[0], offsetY + tr[1], offsetZ + tr[2]), Vector2(w, 0));
		vD = Array::make(Vector3(offsetX + tl[0], offsetY + tl[1], offsetZ + tl[2]), Vector2(0, 0));
	}

	vertices.append(vA);
	vertices.append(vB);
	vertices.append(vC);
	vertices.append(vD);
}

void EcoGame::createLeft(Vector3 offset, int bl[], int tl[], int tr[], int br[], Array vertices, int type) {
	float w = TEXTURE_SCALE * abs(bl[2] - br[2]);
	float h = TEXTURE_SCALE * abs(bl[1] - tl[1]);

	float u = getU(type);
	float v = getV(type);
	float u2 = u + w;
	float v2 = v + h;

	int offsetX = offset.x;
	int offsetY = offset.y;
	int offsetZ = offset.z;

	Array vA;
	Array vB;
	Array vC;
	Array vD;

	if (TEXTURE_MODE == 0) {
		vA = Array::make(Vector3(offsetX + bl[0], offsetY + bl[1], offsetZ + bl[2]), Vector2(u, v2));
		vB = Array::make(Vector3(offsetX + br[0], offsetY + br[1], offsetZ + br[2]), Vector2(u2, v2));
		vC = Array::make(Vector3(offsetX + tr[0], offsetY + tr[1], offsetZ + tr[2]), Vector2(u2, v));
		vD = Array::make(Vector3(offsetX + tl[0], offsetY + tl[1], offsetZ + tl[2]), Vector2(u, v));
	}
	else {
		vA = Array::make(Vector3(offsetX + bl[0], offsetY + bl[1], offsetZ + bl[2]), Vector2(0, h));
		vB = Array::make(Vector3(offsetX + br[0], offsetY + br[1], offsetZ + br[2]), Vector2(w, h));
		vC = Array::make(Vector3(offsetX + tr[0], offsetY + tr[1], offsetZ + tr[2]), Vector2(w, 0));
		vD = Array::make(Vector3(offsetX + tl[0], offsetY + tl[1], offsetZ + tl[2]), Vector2(0, 0));
	}

	vertices.append(vA);
	vertices.append(vB);
	vertices.append(vC);
	vertices.append(vD);
}

void EcoGame::createRight(Vector3 offset, int bl[], int tl[], int tr[], int br[], Array vertices, int type) {
	float w = TEXTURE_SCALE * abs(bl[1] - tl[1]);
	float h = TEXTURE_SCALE * abs(bl[2] - br[2]);

	float u = getU(type);
	float v = getV(type);
	float u2 = u + w;
	float v2 = v + h;
	
	int offsetX = offset.x;
	int offsetY = offset.y;
	int offsetZ = offset.z;

	Array vA;
	Array vB;
	Array vC;
	Array vD;

	if (TEXTURE_MODE == 0) {
		vA = Array::make(Vector3(offsetX + bl[0], offsetY + bl[1], offsetZ + bl[2]), Vector2(u, v2));
		vB = Array::make(Vector3(offsetX + tl[0], offsetY + tl[1], offsetZ + tl[2]), Vector2(u2, v2));
		vC = Array::make(Vector3(offsetX + tr[0], offsetY + tr[1], offsetZ + tr[2]), Vector2(u2, v));
		vD = Array::make(Vector3(offsetX + br[0], offsetY + br[1], offsetZ + br[2]), Vector2(u, v));
	}
	else {
		vA = Array::make(Vector3(offsetX + bl[0], offsetY + bl[1], offsetZ + bl[2]), Vector2(0, h));
		vB = Array::make(Vector3(offsetX + tl[0], offsetY + tl[1], offsetZ + tl[2]), Vector2(w, h));
		vC = Array::make(Vector3(offsetX + tr[0], offsetY + tr[1], offsetZ + tr[2]), Vector2(w, 0));
		vD = Array::make(Vector3(offsetX + br[0], offsetY + br[1], offsetZ + br[2]), Vector2(0, 0));
	}

	vertices.append(vA);
	vertices.append(vB);
	vertices.append(vC);
	vertices.append(vD);
}

void EcoGame::createFront(Vector3 offset, int bl[], int tl[], int tr[], int br[], Array vertices, int type) {
	float w = TEXTURE_SCALE * abs(bl[0] - tl[0]);
	float h = TEXTURE_SCALE * abs(bl[1] - br[1]);

	float u = getU(type);
	float v = getV(type);
	float u2 = u + w;
	float v2 = v + h;
	
	int offsetX = offset.x;
	int offsetY = offset.y;
	int offsetZ = offset.z;

	Array vA;
	Array vB;
	Array vC;
	Array vD;

	if (TEXTURE_MODE == 0) {
		vA = Array::make(Vector3(offsetX + bl[0], offsetY + bl[1], offsetZ + bl[2]), Vector2(u, v2));
		vB = Array::make(Vector3(offsetX + tl[0], offsetY + tl[1], offsetZ + tl[2]), Vector2(u2, v2));
		vC = Array::make(Vector3(offsetX + tr[0], offsetY + tr[1], offsetZ + tr[2]), Vector2(u2, v));
		vD = Array::make(Vector3(offsetX + br[0], offsetY + br[1], offsetZ + br[2]), Vector2(u, v));
	}
	else {
		vA = Array::make(Vector3(offsetX + bl[0], offsetY + bl[1], offsetZ + bl[2]), Vector2(0, h));
		vB = Array::make(Vector3(offsetX + tl[0], offsetY + tl[1], offsetZ + tl[2]), Vector2(w, h));
		vC = Array::make(Vector3(offsetX + tr[0], offsetY + tr[1], offsetZ + tr[2]), Vector2(w, 0));
		vD = Array::make(Vector3(offsetX + br[0], offsetY + br[1], offsetZ + br[2]), Vector2(0, 0));
	}

	vertices.append(vA);
	vertices.append(vB);
	vertices.append(vC);
	vertices.append(vD);
}

void EcoGame::createBack(Vector3 offset, int bl[], int tl[], int tr[], int br[], Array vertices, int type) {
	float w = TEXTURE_SCALE * abs(bl[1] - br[1]);
	float h = TEXTURE_SCALE * abs(bl[0] - tl[0]);

	float u = getU(type);
	float v = getV(type);
	float u2 = u + w;
	float v2 = v + h;
	
	int offsetX = offset.x;
	int offsetY = offset.y;
	int offsetZ = offset.z;

	Array vA;
	Array vB;
	Array vC;
	Array vD;

	if (TEXTURE_MODE == 0) {
		vA = Array::make(Vector3(offsetX + bl[0], offsetY + bl[1], offsetZ + bl[2]), Vector2(u, v2));
		vB = Array::make(Vector3(offsetX + br[0], offsetY + br[1], offsetZ + br[2]), Vector2(u2, v2));
		vC = Array::make(Vector3(offsetX + tr[0], offsetY + tr[1], offsetZ + tr[2]), Vector2(u2, v));
		vD = Array::make(Vector3(offsetX + tl[0], offsetY + tl[1], offsetZ + tl[2]), Vector2(u, v));
	}
	else {
		vA = Array::make(Vector3(offsetX + bl[0], offsetY + bl[1], offsetZ + bl[2]), Vector2(0, h));
		vB = Array::make(Vector3(offsetX + br[0], offsetY + br[1], offsetZ + br[2]), Vector2(w, h));
		vC = Array::make(Vector3(offsetX + tr[0], offsetY + tr[1], offsetZ + tr[2]), Vector2(w, 0));
		vD = Array::make(Vector3(offsetX + tl[0], offsetY + tl[1], offsetZ + tl[2]), Vector2(0, 0));
	}

	vertices.append(vA);
	vertices.append(vB);
	vertices.append(vC);
	vertices.append(vD);
}
