/*

Implementations of Octree member functions.

Copyright (C) 2011  Tao Ju

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public License
(LGPL) as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "octree.h"
#include "density.h"
#include "voxelworld.h"

using namespace godot;

// ----------------------------------------------------------------------------

const int MATERIAL_AIR = 0;
const int MATERIAL_SOLID = 1;

const float QEF_ERROR = 1e-6f;
const int QEF_SWEEPS = 4;

// ----------------------------------------------------------------------------
// data from the original DC impl, drives the contouring process

const int edgevmap[12][2] =
{
	{0,4},{1,5},{2,6},{3,7},	// x-axis 
	{0,2},{1,3},{4,6},{5,7},	// y-axis
	{0,1},{2,3},{4,5},{6,7}		// z-axis
};

const int edgemask[3] = { 5, 3, 6 };

const int vertMap[8][3] =
{
	{0,0,0},
	{0,0,1},
	{0,1,0},
	{0,1,1},
	{1,0,0},
	{1,0,1},
	{1,1,0},
	{1,1,1}
};

const int faceMap[6][4] = { {4, 8, 5, 9}, {6, 10, 7, 11},{0, 8, 1, 10},{2, 9, 3, 11},{0, 4, 2, 6},{1, 5, 3, 7} };
const int cellProcFaceMask[12][3] = { {0,4,0},{1,5,0},{2,6,0},{3,7,0},{0,2,1},{4,6,1},{1,3,1},{5,7,1},{0,1,2},{2,3,2},{4,5,2},{6,7,2} };
const int cellProcEdgeMask[6][5] = { {0,1,2,3,0},{4,5,6,7,0},{0,4,1,5,1},{2,6,3,7,1},{0,2,4,6,2},{1,3,5,7,2} };

const int faceProcFaceMask[3][4][3] = {
	{{4,0,0},{5,1,0},{6,2,0},{7,3,0}},
	{{2,0,1},{6,4,1},{3,1,1},{7,5,1}},
	{{1,0,2},{3,2,2},{5,4,2},{7,6,2}}
};

const int faceProcEdgeMask[3][4][6] = {
	{{1,4,0,5,1,1},{1,6,2,7,3,1},{0,4,6,0,2,2},{0,5,7,1,3,2}},
	{{0,2,3,0,1,0},{0,6,7,4,5,0},{1,2,0,6,4,2},{1,3,1,7,5,2}},
	{{1,1,0,3,2,0},{1,5,4,7,6,0},{0,1,5,0,4,1},{0,3,7,2,6,1}}
};

const int edgeProcEdgeMask[3][2][5] = {
	{{3,2,1,0,0},{7,6,5,4,0}},
	{{5,1,4,0,1},{7,3,6,2,1}},
	{{6,4,2,0,2},{7,5,3,1,2}},
};

const int processEdgeMask[3][4] = { {3,2,1,0},{7,5,6,4},{11,10,9,8} };

void OctreeNode::_register_methods() {
	register_method("getMeshInstanceId", &OctreeNode::getMeshInstanceId);
	register_method("setMeshInstanceId", &OctreeNode::setMeshInstanceId);
}

void OctreeNode::_init() {

}

// ----------------------------------------------------------------------------

std::shared_ptr<OctreeNode> SimplifyOctree(std::shared_ptr<OctreeNode> node, float threshold)
{
	if (!node)
	{
		return NULL;
	}

	if (node->type != Node_Internal)
	{
		// can't simplify!
		return node;
	}

	svd::QefSolver qef;
	int signs[8] = { -1, -1, -1, -1, -1, -1, -1, -1 };
	int midsign = -1;
	int edgeCount = 0;
	bool isCollapsible = true;

	for (int i = 0; i < 8; i++)
	{
		node->children[i] = SimplifyOctree(node->children[i], threshold);
		if (node->children[i])
		{
			std::shared_ptr<OctreeNode> child = node->children[i];
			if (child->type == Node_Internal)
			{
				isCollapsible = false;
			}
			else
			{
				qef.add(child->drawInfo->qef);

				midsign = (child->drawInfo->corners >> (7 - i)) & 1;
				signs[i] = (child->drawInfo->corners >> i) & 1;

				edgeCount++;
			}
		}
	}

	if (!isCollapsible)
	{
		// at least one child is an internal node, can't collapse
		return node;
	}

	svd::Vec3 qefPosition;
	qef.solve(qefPosition, QEF_ERROR, QEF_SWEEPS, QEF_ERROR);
	float error = qef.getError();

	// convert to glm Vector3 for ease of use
	Vector3 position(qefPosition.x, qefPosition.y, qefPosition.z);

	// at this point the masspoint will actually be a sum, so divide to make it the average
	if (error > threshold)
	{
		// this collapse breaches the threshold
		return node;
	}

	if (position.x < node->min.x || position.x >(node->min.x + node->size) ||
		position.y < node->min.y || position.y >(node->min.y + node->size) ||
		position.z < node->min.z || position.z >(node->min.z + node->size))
	{
		const auto& mp = qef.getMassPoint();
		position = Vector3(mp.x, mp.y, mp.z);
	}

	// change the node from an internal node to a 'psuedo leaf' node
	std::shared_ptr<OctreeDrawInfo> drawInfo = make_shared<OctreeDrawInfo>();

	for (int i = 0; i < 8; i++)
	{
		if (signs[i] == -1)
		{
			// Undetermined, use centre sign instead
			drawInfo->corners |= (midsign << i);
		}
		else
		{
			drawInfo->corners |= (signs[i] << i);
		}
	}

	drawInfo->averageNormal = Vector3(0, 0, 0);
	for (int i = 0; i < 8; i++)
	{
		if (node->children[i])
		{
			std::shared_ptr<OctreeNode> child = node->children[i];
			if (child->type == Node_Pseudo ||
				child->type == Node_Leaf)
			{
				drawInfo->averageNormal += child->drawInfo->averageNormal;
			}
		}
	}

	drawInfo->averageNormal = (drawInfo->averageNormal).normalized();
	drawInfo->position = position;
	drawInfo->qef = qef.getData();

	for (int i = 0; i < 8; i++)
	{
		DestroyOctree(node->children[i]);
		node->children[i] = nullptr;
	}

	node->type = Node_Pseudo;
	node->drawInfo = drawInfo;

	return node;
}

// ----------------------------------------------------------------------------

void GenerateVertexIndices(std::shared_ptr<OctreeNode> node, VertexBuffer& vertexBuffer, int* counts)
{
	if (!node)
	{
		return;
	}

	if (node->type != Node_Leaf)
	{
		for (int i = 0; i < 8; i++)
		{
			GenerateVertexIndices(node->children[i], vertexBuffer, counts);
		}
	}

	if (node->type != Node_Internal)
	{
		std::shared_ptr<OctreeDrawInfo> d = node->drawInfo;
		if (!d)
		{
			printf("Error! Could not add vertex!\n");
			exit(EXIT_FAILURE);
		}

		d->index = counts[0];
		vertexBuffer[counts[0]++] = MeshVertex(d->position, d->averageNormal);
	}
}

// ----------------------------------------------------------------------------

void ContourProcessEdge(std::shared_ptr<OctreeNode> node[4], int dir, IndexBuffer& indexBuffer, int* counts)
{
	int minSize = 1000000;		// arbitrary big number
	int minIndex = 0;
	int indices[4] = { -1, -1, -1, -1 };
	bool flip = false;
	bool signChange[4] = { false, false, false, false };

	for (int i = 0; i < 4; i++)
	{
		const int edge = processEdgeMask[dir][i];
		const int c1 = edgevmap[edge][0];
		const int c2 = edgevmap[edge][1];

		const int m1 = (node[i]->drawInfo->corners >> c1) & 1;
		const int m2 = (node[i]->drawInfo->corners >> c2) & 1;

		if (node[i]->size < minSize)
		{
			minSize = node[i]->size;
			minIndex = i;
			flip = m1 != MATERIAL_AIR;
		}

		indices[i] = node[i]->drawInfo->index;

		signChange[i] =
			(m1 == MATERIAL_AIR && m2 != MATERIAL_AIR) ||
			(m1 != MATERIAL_AIR && m2 == MATERIAL_AIR);
	}

	if (signChange[minIndex])
	{
		if (!flip)
		{
			indexBuffer[counts[1]++] = indices[0];
			indexBuffer[counts[1]++] = indices[1];
			indexBuffer[counts[1]++] = indices[3];

			indexBuffer[counts[1]++] = indices[0];
			indexBuffer[counts[1]++] = indices[3];
			indexBuffer[counts[1]++] = indices[2];
		}
		else
		{
			indexBuffer[counts[1]++] = indices[0];
			indexBuffer[counts[1]++] = indices[3];
			indexBuffer[counts[1]++] = indices[1];

			indexBuffer[counts[1]++] = indices[0];
			indexBuffer[counts[1]++] = indices[2];
			indexBuffer[counts[1]++] = indices[3];
		}
	}
}

// ----------------------------------------------------------------------------

void ContourEdgeProc(std::shared_ptr<OctreeNode> node[4], int dir, IndexBuffer& indexBuffer, int* counts)
{
	if (!node[0] || !node[1] || !node[2] || !node[3])
	{
		return;
	}

	if (node[0]->type != Node_Internal &&
		node[1]->type != Node_Internal &&
		node[2]->type != Node_Internal &&
		node[3]->type != Node_Internal)
	{
		ContourProcessEdge(node, dir, indexBuffer, counts);
	}
	else
	{
		for (int i = 0; i < 2; i++)
		{
			std::shared_ptr<OctreeNode> edgeNodes[4];
			const int c[4] =
			{
				edgeProcEdgeMask[dir][i][0],
				edgeProcEdgeMask[dir][i][1],
				edgeProcEdgeMask[dir][i][2],
				edgeProcEdgeMask[dir][i][3],
			};

			for (int j = 0; j < 4; j++)
			{
				if (node[j]->type == Node_Leaf || node[j]->type == Node_Pseudo)
				{
					edgeNodes[j] = node[j];
				}
				else
				{
					edgeNodes[j] = node[j]->children[c[j]];
				}
			}

			ContourEdgeProc(edgeNodes, edgeProcEdgeMask[dir][i][4], indexBuffer, counts);
		}
	}
}

// ----------------------------------------------------------------------------

void ContourFaceProc(std::shared_ptr<OctreeNode> node[2], int dir, IndexBuffer& indexBuffer, int* counts)
{
	if (!node[0] || !node[1])
	{
		return;
	}

	if (node[0]->type == Node_Internal ||
		node[1]->type == Node_Internal)
	{
		for (int i = 0; i < 4; i++)
		{
			std::shared_ptr<OctreeNode> faceNodes[2];
			const int c[2] =
			{
				faceProcFaceMask[dir][i][0],
				faceProcFaceMask[dir][i][1],
			};

			for (int j = 0; j < 2; j++)
			{
				if (node[j]->type != Node_Internal)
				{
					faceNodes[j] = node[j];
				}
				else
				{
					faceNodes[j] = node[j]->children[c[j]];
				}
			}

			ContourFaceProc(faceNodes, faceProcFaceMask[dir][i][2], indexBuffer, counts);
		}

		const int orders[2][4] =
		{
			{ 0, 0, 1, 1 },
			{ 0, 1, 0, 1 },
		};
		for (int i = 0; i < 4; i++)
		{
			std::shared_ptr<OctreeNode> edgeNodes[4];
			const int c[4] =
			{
				faceProcEdgeMask[dir][i][1],
				faceProcEdgeMask[dir][i][2],
				faceProcEdgeMask[dir][i][3],
				faceProcEdgeMask[dir][i][4],
			};

			const int* order = orders[faceProcEdgeMask[dir][i][0]];
			for (int j = 0; j < 4; j++)
			{
				if (node[order[j]]->type == Node_Leaf ||
					node[order[j]]->type == Node_Pseudo)
				{
					edgeNodes[j] = node[order[j]];
				}
				else
				{
					edgeNodes[j] = node[order[j]]->children[c[j]];
				}
			}

			ContourEdgeProc(edgeNodes, faceProcEdgeMask[dir][i][5], indexBuffer, counts);
		}
	}
}

// ----------------------------------------------------------------------------

void ContourCellProc(std::shared_ptr<OctreeNode> node, IndexBuffer& indexBuffer, int* counts)
{
	if (node == NULL)
	{
		return;
	}

	if (node->type == Node_Internal)
	{
		for (int i = 0; i < 8; i++)
		{
			ContourCellProc(node->children[i], indexBuffer, counts);
		}

		for (int i = 0; i < 12; i++)
		{
			std::shared_ptr<OctreeNode> faceNodes[2];
			const int c[2] = { cellProcFaceMask[i][0], cellProcFaceMask[i][1] };

			faceNodes[0] = node->children[c[0]];
			faceNodes[1] = node->children[c[1]];

			ContourFaceProc(faceNodes, cellProcFaceMask[i][2], indexBuffer, counts);
		}

		for (int i = 0; i < 6; i++)
		{
			std::shared_ptr<OctreeNode> edgeNodes[4];
			const int c[4] =
			{
				cellProcEdgeMask[i][0],
				cellProcEdgeMask[i][1],
				cellProcEdgeMask[i][2],
				cellProcEdgeMask[i][3],
			};

			for (int j = 0; j < 4; j++)
			{
				edgeNodes[j] = node->children[c[j]];
			}

			ContourEdgeProc(edgeNodes, cellProcEdgeMask[i][4], indexBuffer, counts);
		}
	}
}

// ----------------------------------------------------------------------------

float Density_Func(Vector3 v) {
	float size2 = pow(2, MAX_LOD) * CHUNK_SIZE / 2;
	Vector3 origin(size2, size2, size2);
	return Sphere(v, origin, size2);
}

// ----------------------------------------------------------------------------

Vector3 ApproximateZeroCrossingPosition(const Vector3& p0, const Vector3& p1) {
	// approximate the zero crossing by finding the min value along the edge
	float minValue = 100000.f;
	float t = 0.f;
	float currentT = 0.f;
	const int steps = 8;
	const float increment = 1.f / (float)steps;
	while (currentT <= 1.f) {
		const Vector3 p = p0 + ((p1 - p0) * currentT);
		const float density = abs(Density_Func(p));
		if (density < minValue) {
			minValue = density;
			t = currentT;
		}

		currentT += increment;
	}

	return p0 + ((p1 - p0) * t);
}

// ----------------------------------------------------------------------------

Vector3 CalculateSurfaceNormal(const Vector3 p) {
	const float H = 0.001f;
	const float dx = Density_Func(p + Vector3(H, 0.f, 0.f)) - Density_Func(p - Vector3(H, 0.f, 0.f));
	const float dy = Density_Func(p + Vector3(0.f, H, 0.f)) - Density_Func(p - Vector3(0.f, H, 0.f));
	const float dz = Density_Func(p + Vector3(0.f, 0.f, H)) - Density_Func(p - Vector3(0.f, 0.f, H));

	return Vector3(dx, dy, dz).normalized();
}

// ----------------------------------------------------------------------------

vector<std::shared_ptr<OctreeNode>> FindActiveVoxels(std::shared_ptr<OctreeNode> reference) {
	vector<std::shared_ptr<OctreeNode>> leafs;
	const Vector3 min = reference->min;
	const int cellSize = pow(2, reference->lod);

	bool hasLowerLod = false;
	for (int i = 0; i < 8; i++) {
		auto child = reference->children[i];
		if (!child || child->lod < 0) continue;
		hasLowerLod = true;
		auto nodes = FindActiveVoxels(child);
		leafs.insert(end(leafs), begin(nodes), end(nodes));
	}
	if (hasLowerLod) return leafs;

	for (int x = 0; x < reference->size; x += cellSize)
		for (int y = 0; y < reference->size; y += cellSize)
			for (int z = 0; z < reference->size; z += cellSize)
			{
				const Vector3 idxPos(x, y, z);
				const Vector3 pos = idxPos + min;

				int corners = 0;
				for (int i = 0; i < 8; i++)
				{
					const Vector3 cornerPos = pos + (CHILD_MIN_OFFSETS[i] * cellSize);
					const float density = Density_Func(cornerPos);
					const int material = density < 0.f ? MATERIAL_SOLID : MATERIAL_AIR;
					corners |= (material << i);
				}

				if (corners == 0 || corners == 255)
				{
					continue;
				}

				// otherwise the voxel contains the surface, so find the edge intersections
				const int MAX_CROSSINGS = 6;
				Vector3 averageNormal(0, 0, 0);
				svd::QefSolver qef;

				int idx = 0;

				for (int i = 0; i < 12 && idx < MAX_CROSSINGS; i++)
				{
					const int c1 = edgevmap[i][0];
					const int c2 = edgevmap[i][1];

					const int m1 = (corners >> c1) & 1;
					const int m2 = (corners >> c2) & 1;

					if ((m1 == MATERIAL_AIR && m2 == MATERIAL_AIR) ||
						(m1 == MATERIAL_SOLID && m2 == MATERIAL_SOLID))
					{
						// no zero crossing on this edge
						continue;
					}

					const Vector3 p1 = Vector3(pos + (CHILD_MIN_OFFSETS[c1] * cellSize));
					const Vector3 p2 = Vector3(pos + (CHILD_MIN_OFFSETS[c2] * cellSize));
					const Vector3 p = ApproximateZeroCrossingPosition(p1, p2);
					const Vector3 n = CalculateSurfaceNormal(p);
					qef.add(p.x, p.y, p.z, n.x, n.y, n.z);

					averageNormal += n;

					idx++;
				}
				
				svd::Vec3 qefPosition;
				qef.solve(qefPosition, QEF_ERROR, QEF_SWEEPS, QEF_ERROR);

				std::shared_ptr<OctreeDrawInfo> drawInfo = make_shared<OctreeDrawInfo>();
				drawInfo->position = Vector3(qefPosition.x, qefPosition.y, qefPosition.z);
				drawInfo->qef = qef.getData();

				const Vector3 min = pos;
				const Vector3 max = pos + Vector3(cellSize, cellSize, cellSize);
				if (drawInfo->position.x < min.x || drawInfo->position.x > max.x ||
					drawInfo->position.y < min.y || drawInfo->position.y > max.y ||
					drawInfo->position.z < min.z || drawInfo->position.z > max.z)
				{
					const auto& mp = qef.getMassPoint();
					drawInfo->position = Vector3(mp.x, mp.y, mp.z);
				}

				drawInfo->averageNormal = (averageNormal / (float)idx).normalized();
				drawInfo->corners = corners;

				std::shared_ptr<OctreeNode> node = shared_ptr<OctreeNode>(OctreeNode::_new());
				node->min = pos;
				node->size = cellSize;
				node->type = Node_Leaf;
				node->drawInfo = drawInfo;
				leafs.push_back(node);
			}
	
	return leafs;
}

// -------------------------------------------------------------------------------

vector<std::shared_ptr<OctreeNode>> FindLodNodes(std::shared_ptr<OctreeNode> node) {
	vector<std::shared_ptr<OctreeNode>> lodNodes;
	bool childFound = false;

	for (int i = 0; i < 8; i++) {
		auto child = node->children[i];
		if (!child || child->lod < 0) continue;
		childFound = true;
		auto nodes = FindLodNodes(child);
		lodNodes.insert(end(lodNodes), begin(nodes), end(nodes));
	}

	if (!childFound) {
		lodNodes.push_back(node);
	}

	return lodNodes;
}

// -------------------------------------------------------------------------------
float Squared(float v) { return v * v; };

bool BoxIntersectsSphere(Vector3 BminV, Vector3 BmaxV, Vector3 CV, float r) {
	float r2 = r * r;
	int i, dmin = 0;
	float Bmin[3] = { BminV.x, BminV.y, BminV.z };
	float Bmax[3] = { BmaxV.x, BmaxV.y, BmaxV.z };
	float C[3]	  = { CV.x, CV.y, CV.z };
	
	for (i = 0; i < 3; i++) {
		if (C[i] < Bmin[i]) dmin += Squared(C[i] - Bmin[i]);
		else if (C[i] > Bmax[i]) dmin += Squared(C[i] - Bmax[i]);
	}
	return dmin <= r2;
}

void ExpandNodes(std::shared_ptr<OctreeNode> node, Vector3 center, float range) {
	int i;
	const int childLod = node->lod - 1;
	const int size2 = node->size / 2;
	std::shared_ptr<OctreeNode> child;
	Vector3 nodeCenter;

	nodeCenter = node->min + Vector3(size2, size2, size2);
	if (!BoxIntersectsSphere(node->min, node->min + Vector3(node->size, node->size, node->size), center, range)) return;

	for (i = 0; i < 8; i++) {
		if (!node->children[i]) {
			child = shared_ptr<OctreeNode>(OctreeNode::_new());
			child->size = size2;
			child->min = node->min + (CHILD_MIN_OFFSETS[i] * size2);
			child->lod = childLod;
			child->type = Node_Internal;
			child->meshInstanceId = node->meshInstanceId;
			child->dirty = true;

			node->meshInstanceId = 0;
			node->dirty = false;
			node->children[i] = child;
			continue;
		}
		else {
			child = node->children[i];
			if (childLod > 0)
				ExpandNodes(child, center, range);
		}
	}
}

// -------------------------------------------------------------------------------

void ExpandNodes(std::shared_ptr<OctreeNode> node, int lod) {
	int i;
	const int childLod = node->lod - 1;
	const int childSize = node->size / 2;
	std::shared_ptr<OctreeNode> child;

	for (i = 0; i < 8; i++) {
		if (!node->children[i]) {
			child = shared_ptr<OctreeNode>(OctreeNode::_new());
			child->size = childSize;
			child->min = node->min + (CHILD_MIN_OFFSETS[i] * childSize);
			child->lod = childLod;
			child->type = Node_Internal;

			node->children[i] = child;
		}
		else {
			child = node->children[i];
		}

		if (childLod != lod)
			ExpandNodes(child, lod);
	}
}

// ----------------------------------------------------------------------------

void GenerateMeshFromOctree(std::shared_ptr<OctreeNode> node, VertexBuffer& vertexBuffer, IndexBuffer& indexBuffer, int* counts)
{
	if (!node)
	{
		return;
	}

	GenerateVertexIndices(node, vertexBuffer, counts);
	ContourCellProc(node, indexBuffer, counts);
}

// -------------------------------------------------------------------------------

void DestroyOctree(std::shared_ptr<OctreeNode> node)
{
	if (!node)
	{
		return;
	}

	for (int i = 0; i < 8; i++)
	{
		DestroyOctree(node->children[i]);
	}

	if (node->drawInfo)
	{
		node->drawInfo.reset();
	}

	node.reset();
}

// -------------------------------------------------------------------------------

vector<std::shared_ptr<OctreeNode>> FindSeamNodes(std::shared_ptr<VoxelWorld> world, Vector3 baseChunkMin)
{
	const Vector3 seamValues = baseChunkMin + Vector3(CHUNK_SIZE, CHUNK_SIZE, CHUNK_SIZE);
	const Vector3 OFFSETS[8] =
	{
		Vector3(0,0,0), Vector3(1,0,0), Vector3(0,0,1), Vector3(1,0,1),
		Vector3(0,1,0), Vector3(1,1,0), Vector3(0,1,1), Vector3(1,1,1)
	};

	FilterNodesFunc selectionFuncs[8] =
	{
		[&](const Vector3& min, const Vector3& max)
		{
			return max.x == seamValues.x || max.y == seamValues.y || max.z == seamValues.z;
		},

		[&](const Vector3& min, const Vector3& max)
		{
			return min.x == seamValues.x;
		},

		[&](const Vector3& min, const Vector3& max)
		{
			return min.z == seamValues.z;
		},

		[&](const Vector3& min, const Vector3& max)
		{
			return min.x == seamValues.x && min.z == seamValues.z;
		},

		[&](const Vector3& min, const Vector3& max)
		{
			return min.y == seamValues.y;
		},

		[&](const Vector3& min, const Vector3& max)
		{
			return min.x == seamValues.x && min.y == seamValues.y;
		},

		[&](const Vector3& min, const Vector3& max)
		{
			return min.y == seamValues.y && min.z == seamValues.z;
		},

		[&](const Vector3& min, const Vector3& max)
		{
			return min.x == seamValues.x && min.y == seamValues.y && min.z == seamValues.z;
		}
	};

	vector<std::shared_ptr<OctreeNode>> seamNodes;
	for (int i = 0; i < 8; i++)
	{
		const Vector3 offsetMin = OFFSETS[i] * CHUNK_SIZE;
		const Vector3 chunkMin = baseChunkMin + offsetMin;
		/*auto c = world->getChunk(chunkMin);
		if (c)
		{
			auto chunkNodes = c->findNodes(selectionFuncs[i]);
			seamNodes.insert(end(seamNodes), begin(chunkNodes), end(chunkNodes));
		}*/
	}

	return seamNodes;
}

// -------------------------------------------------------------------------------

void Octree_FindNodes(std::shared_ptr<OctreeNode> node, FilterNodesFunc& func, vector<std::shared_ptr<OctreeNode>>& nodes)
{
	if (!node)
	{
		return;
	}

	const Vector3 max = node->min + Vector3(node->size, node->size, node->size);
	if (!func(node->min, max))
	{
		return;
	}

	if (node->type == Node_Leaf || node->type == Node_Pseudo)
	{
		nodes.push_back(node->cpy());
	}
	else
	{
		for (int i = 0; i < 8; i++)
			Octree_FindNodes(node->children[i], func, nodes);
	}
}

// -------------------------------------------------------------------------------

std::shared_ptr<OctreeNode> BuildOctree(vector<std::shared_ptr<OctreeNode>> seams, Vector3 chunkMin, int parentSize) {
	vector<std::shared_ptr<OctreeNode>> parents;

	sort(seams.begin(), seams.end(), [](const std::shared_ptr<OctreeNode>& a, const std::shared_ptr<OctreeNode>& b) {
		return a->size > b->size;
	});

	for (auto node : seams) {
		if (node->size * 2 != parentSize) {
			parents.push_back(node);
			continue;
		}

		Vector3 parentMin = node->min - Vector3(
			int(node->min.x - chunkMin.x) % parentSize,
			int(node->min.y - chunkMin.y) % parentSize,
			int(node->min.z - chunkMin.z) % parentSize);
		bool hasElement = false;
		int index = -1;

		for (int i = 0; i < 8; i++) {
			Vector3 childMin = parentMin + (node->size * CHILD_MIN_OFFSETS[i]);
			if (childMin == node->min) {
				index = i;
				break;
			}
		}
		
		if (index < 0) continue;
		
		if (!hasElement) {
			for (auto parent : parents) {
				if (parent->min == parentMin) {
					hasElement = true;
					parent->children[index] = node;
					break;
				}
			}
		}

		if (!hasElement) {
			auto parent = shared_ptr<OctreeNode>(OctreeNode::_new());
			parent->min = parentMin;
			parent->size = parentSize;
			parent->type = Node_Internal;
			parents.push_back(parent);
			parent->children[index] = node;
		}
	}

	if (parents.size() == 1) return parents.front();
	return BuildOctree(parents, chunkMin, parentSize * 2);
}

// -------------------------------------------------------------------------------
