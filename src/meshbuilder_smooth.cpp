#include "meshbuilder_smooth.h"
#include "graphnode.h"
#include "navigator.h"
#include "threadpool.h"
#include "ecogame.h"

using namespace godot;

MeshBuilder_Smooth::MeshBuilder_Smooth() {
	initCubeEdges();
	initEdgeTable();
}

MeshBuilder_Smooth::~MeshBuilder_Smooth() {
	delete[] cube_edges;
	delete[] edge_table;
}

void MeshBuilder_Smooth::initCubeEdges() {
	int i, j, p;
	int k = 0;
	for (i = 0; i < 8; ++i) {
		for (j = 1; j <= 4; j <<= 1) {
			p = i ^ j;
			if (i <= p) {
				cube_edges[k++] = i;
				cube_edges[k++] = p;
			}
		}
	}
}

/**
 * Initializes the cube edge table. This follows the idea of Paul Bourke
 * link: http://paulbourke.net/geometry/polygonise/
 */
void MeshBuilder_Smooth::initEdgeTable() {
	int i, j, em;
	bool a, b;
	for (i = 0; i < 256; ++i) {
		em = 0;
		for (j = 0; j < 24; j += 2) {
			a = bool(i & (1 << cube_edges[j]));
			b = bool(i & (1 << cube_edges[j + 1]));
			em |= a != b ? (1 << (j >> 1)) : 0;
		}
		edge_table[i] = em;
	}
}

Array MeshBuilder_Smooth::buildVertices(std::shared_ptr<Chunk> chunk) {
	// location (location[0]=x, location[1]=y, location[2]=z)
	int location[3];
	// layout for one-dimensional data array
	// we use this to reference vertex buffer

	std::shared_ptr<VoxelData> volume = chunk->getVolume();
	Vector3 offset = chunk->getOffset();
	int vw = volume->getWidth();
	int vh = volume->getHeight();
	int vd = volume->getDepth();
	int vx, vy, vz, vv, vv1, vertsCount = 0, facesCount = 0;

	int pd = 2;
	int pd2 = pd / 2;
	int DIMS[3] = {
		vw + pd,
		vh + pd,
		vd + pd
	};

	int R[3] = {
		// x
		1,
		// y * width
		DIMS[0] + 1,
		// z * width * height
		(DIMS[0] + 1) * (DIMS[1] + 1)
	};
	// grid cell
	double grid[8];

	int buf_no = 1;
	int n = 0;

	const int VERTEX_BUFFER_SIZE = R[2] * 2;
	int* vertexBuffer = new int[VERTEX_BUFFER_SIZE];

	Array vertices, faces;
	vertices.resize(VERTEX_BUFFER_SIZE);
	faces.resize(VERTEX_BUFFER_SIZE);

	// March over the voxel grid
	for (location[2] = 0; location[2] < DIMS[2] - 1; ++location[2], n += DIMS[0], buf_no ^= 1 /*even or odd*/, R[2] = -R[2]) {

		// m is the pointer into the buffer we are going to use.  
		// This is slightly obtuse because javascript does not
		// have good support for packed data structures, so we must
		// use typed arrays :(
		// The contents of the buffer will be the indices of the
		// vertices on the previous x/y slice of the volume
		int m = 1 + (DIMS[0] + 1) * (1 + buf_no * (DIMS[1] + 1));

		for (location[1] = 0; location[1] < DIMS[1] - 1; ++location[1], ++n, m += 2) {
			for (location[0] = 0; location[0] < DIMS[0] - 1; ++location[0], ++n, ++m) {

				// Read in 8 field values around this vertex
				// and store them in an array
				// Also calculate 8-bit mask, like in marching cubes,
				// so we can speed up sign checks later
				int mask = 0, g = 0;
				for (int k = 0; k < 2; ++k) {
					for (int j = 0; j < 2; ++j) {
						for (int i = 0; i < 2; ++i, ++g) {
							vx = location[0] + i - pd2;
							vy = location[1] + j - pd2;
							vz = location[2] + k - pd2;

							/*vv = (int)chunk->isVoxel(vx, vy, vz);
							vv1 = chunk->getVoxel(
								vx % CHUNK_SIZE_X,
								vy % CHUNK_SIZE_Y,
								vz % CHUNK_SIZE_Z);
							vv1 = vv1 > 0 ? 1 : 0;

							if (vv != vv1) {
								cout << "(" << vx << ", " << vy << ", " << vz << "): " << vv << "/" << vv1 << endl;
							}*/

							if (vx < 0 || vx >= vw || vy < 0 || vy >= vh || vz < 0 || vz >= vd) {
								auto neighbour = EcoGame::get()->getChunk(Vector3(vx, vy, vz));
								if (!neighbour) {
									vv = neighbour->getVoxel(
										vx % CHUNK_SIZE_X, 
										vy % CHUNK_SIZE_Y, 
										vz % CHUNK_SIZE_Z);
								}
								else {
									vv = (int)chunk->isVoxel(vx, vy, vz);
								}
							}
							else {
								vv = chunk->getVoxel(
									vx % CHUNK_SIZE_X,
									vy % CHUNK_SIZE_Y,
									vz % CHUNK_SIZE_Z);
							}

							double p = (vv > 0) ? -1.0 : 0.0;
							grid[g] = p;
							mask |= (p < 0) ? (1 << g) : 0;
						}
					}
				}

				// Check for early termination
				// if cell does not intersect boundary
				if (mask == 0 || mask == 0xff) {
					continue;
				}

				// Sum up edge intersections
				int edge_mask = edge_table[mask];
				double v[3];
				int e_count = 0;

				// For every edge of the cube...
				for (int i = 0; i < 12; ++i) {

					// Use edge mask to check if it is crossed
					if (!bool((edge_mask & (1 << i)))) {
						continue;
					}

					// If it did, increment number of edge crossings
					++e_count;

					// Now find the point of intersection
					int firstEdgeIndex = i << 1;
					int secondEdgeIndex = firstEdgeIndex + 1;
					// Unpack vertices
					int e0 = cube_edges[firstEdgeIndex];
					int e1 = cube_edges[secondEdgeIndex];
					// Unpack grid values
					double g0 = grid[e0];
					double g1 = grid[e1];

					// Compute point of intersection (linear interpolation)
					double t = g0 - g1;
					if (abs(t) > 1e-6) {
						t = g0 / t;
					}
					else {
						continue;
					}

					// Interpolate vertices and add up intersections
					// (this can be done without multiplying)
					for (int j = 0; j < 3; j++) {
						int k = 1 << j; // (1,2,4)
						int a = e0 & k;
						int b = e1 & k;
						if (a != b) {
							v[j] += bool(a) ? 1.0 - t : t;
						}
						else {
							v[j] += bool(a) ? 1.0 : 0;
						}
					}
				}

				// Now we just average the edge intersections
				// and add them to coordinate
				double s = 1.0 / e_count;
				for (int i = 0; i < 3; ++i) {
					v[i] = location[i] + s * v[i];
				}

				PoolRealArray vertex;
				vertex.resize(3);
				PoolRealArray::Write vertexWrite = vertex.write();
				vertexWrite[0] = v[0] + offset.x;
				vertexWrite[1] = v[1] + offset.y;
				vertexWrite[2] = v[2] + offset.z;

				// Add vertex to buffer, store pointer to
				// vertex index in buffer
				vertexBuffer[m] = vertsCount;
				vertices[vertsCount++] = vertex;

				// Now we need to add faces together, to do this we just
				// loop over 3 basis components
				for (int i = 0; i < 3; ++i) {

					// The first three entries of the edge_mask
					// count the crossings along the edge
					if (!bool(edge_mask & (1 << i))) {
						continue;
					}

					// i = axes we are point along.
					// iu, iv = orthogonal axes
					int iu = (i + 1) % 3;
					int iv = (i + 2) % 3;

					// If we are on a boundary, skip it
					if (location[iu] == 0 || location[iv] == 0) {
						continue;
					}

					// Otherwise, look up adjacent edges in buffer
					int du = R[iu];
					int dv = R[iv];

					// finally, the indices for the 4 vertices
					// that define the face
					int indexM = vertexBuffer[m];
					int indexMMinusDU = vertexBuffer[m - du];
					int indexMMinusDV = vertexBuffer[m - dv];
					int indexMMinusDUMinusDV = vertexBuffer[m - du - dv];

					// Remember to flip orientation depending on the sign
					// of the corner.
					PoolIntArray face;
					face.resize(4);
					PoolIntArray::Write faceWrite = face.write();

					if (bool(mask & 1)) {
						faceWrite[0] = indexM;
						faceWrite[1] = indexMMinusDU;
						faceWrite[2] = indexMMinusDUMinusDV;
						faceWrite[3] = indexMMinusDV;
					}
					else {
						faceWrite[0] = indexM;
						faceWrite[1] = indexMMinusDV;
						faceWrite[2] = indexMMinusDUMinusDV;
						faceWrite[3] = indexMMinusDU;
					}

					faces[facesCount++] = face;
				}
			} // end x
		} // end y
	} // end z

	//All done!  Return the result
	return Array::make(vertices, faces, vertsCount, facesCount);
}

Array MeshBuilder_Smooth::buildVertices2(std::shared_ptr<Chunk> chunk) {
	// location (location[0]=x, location[1]=y, location[2]=z)
	int x[3];
	// layout for one-dimensional data array
	// we use this to reference vertex buffer

	std::shared_ptr<VoxelData> volume = chunk->getVolume();
	Vector3 offset = chunk->getOffset();
	int vw = volume->getWidth();
	int vh = volume->getHeight();
	int vd = volume->getDepth();
	int vx, vy, vz, vv, vv1, vertsCount = 0, facesCount = 0;
	int DIMS[3] = {
		vw,
		vh,
		vd
	};

	// grid cell
	double grid[8];

	int n = 0;

	const int VERTEX_BUFFER_SIZE = vw * vh * vd * 6 * 4;

	Array vertices, faces;
	vertices.resize(VERTEX_BUFFER_SIZE);
	faces.resize(VERTEX_BUFFER_SIZE);

	int edges[12];

	//March over the volume
	for (x[2] = 0; x[2] < DIMS[2] - 1; ++x[2], n += DIMS[0])
		for (x[1] = 0; x[1] < DIMS[1] - 1; ++x[1], ++n)
			for (x[0] = 0; x[0] < DIMS[0] - 1; ++x[0], ++n) {
				//For each cell, compute cube mask
				int cube_index = 0;
				for (int i = 0; i < 8; ++i) {
					int v[3];
					v[0] = cubeVerts[i][0] + x[0];
					v[1] = cubeVerts[i][1] + x[1];
					v[2] = cubeVerts[i][2] + x[2];

					double s = volume->get(v[0], v[1], v[2]);
					grid[i] = s;
					cube_index |= (s > 0) ? 1 << i : 0;
				}

				//Compute vertices
				int edge_mask = edgeTable[cube_index];
				if (edge_mask == 0) {
					continue;
				}
				
				for (int i = 0; i < 12; ++i) {
					if (!(edge_mask & (1 << i))) {
						continue;
					}
					edges[i] = vertsCount;

					PoolRealArray nv;
					nv.resize(3);
					PoolRealArray::Write nvWrite = nv.write();

					int e[2];
					e[0] = edgeIndex[i][0];
					e[1] = edgeIndex[i][1];

					int p0[3];
					p0[0] = cubeVerts[e[0]][0];
					p0[1] = cubeVerts[e[0]][1];
					p0[2] = cubeVerts[e[0]][2];

					int p1[3];
					p1[0] = cubeVerts[e[1]][0];
					p1[1] = cubeVerts[e[1]][1];
					p1[2] = cubeVerts[e[1]][2];

					double a = grid[e[0]];
					double b = grid[e[1]];
					double d = a - b;
					double t = 0;

					if (abs(d) > 1e-6) {
						t = a / d;
					}
					nvWrite[0] = ((x[0] + p0[0]) + t * (p1[0] - p0[0])) + offset.x;
					nvWrite[1] = ((x[1] + p0[1]) + t * (p1[1] - p0[1])) + offset.y;
					nvWrite[2] = ((x[2] + p0[2]) + t * (p1[2] - p0[2])) + offset.z;
					vertices[vertsCount++] = nv;
				}

				int f[16];
				for (int i = 0; i < 16; i++)
					f[i] = triTable[cube_index][i];
				
				PoolIntArray face;
				face.resize(3);
				PoolIntArray::Write faceWrite = face.write();

				//Add faces
				for (int i = 0; i < 16; i += 3) {
					if (f[i] < 0) break;

					faceWrite[0] = edges[f[i]];
					faceWrite[1] = edges[f[i + 1]];
					faceWrite[2] = edges[f[i + 2]];
					faces[facesCount++] = face;
				}
			}
	return Array::make(vertices, faces, vertsCount, facesCount);
}