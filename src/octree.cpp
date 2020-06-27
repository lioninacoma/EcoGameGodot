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
#include "chunk.h"

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
			if (child->type == Node_Psuedo ||
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

	node->type = Node_Psuedo;
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
				if (node[j]->type == Node_Leaf || node[j]->type == Node_Psuedo)
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
					node[order[j]]->type == Node_Psuedo)
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

float Density_Func(std::shared_ptr<godot::Chunk> chunk, Vector3 v) {
	/*double s = 1.0;

	const int worldSize = chunk->getWorld()->getSize() * CHUNK_SIZE;
	const Vector3 chunkMin = chunk->getOffset();
	const Vector3 n = v - chunkMin;
	int x = n.x;
	int y = n.y;
	int z = n.z;*/

	/*if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_SIZE || z < 0 || z >= CHUNK_SIZE) {
		const Vector3 neighbourMin = chunk->getOffset() + Vector3(x, y, z);

		if (neighbourMin.x < 0 || neighbourMin.x >= worldSize
			|| neighbourMin.z < 0 || neighbourMin.z >= worldSize)
			return s;

		auto neighbour = chunk->getWorld()->getChunk(neighbourMin);
		if (neighbour) {
			s = neighbour->getVoxel(
				(x + CHUNK_SIZE) % CHUNK_SIZE,
				(y + CHUNK_SIZE) % CHUNK_SIZE,
				(z + CHUNK_SIZE) % CHUNK_SIZE);
		}
		else {
			s = chunk->isVoxel(x, y, z);
		}
	}
	else {
		s = chunk->getVoxel(
			x % CHUNK_SIZE,
			y % CHUNK_SIZE,
			z % CHUNK_SIZE);
	}

	return s;*/
	return chunk->isVoxel(v);
	//double noise = FastNoise::GetPerlin(x, y, z) * 0.5 + 0.5;
	//return Sphere(v, Vector3(worldSize / 2, worldSize / 2, worldSize / 2), worldSize / 2);
	//return s = c->getVoxel(
	//	x % CHUNK_SIZE,
	//	y % CHUNK_SIZE,
	//	z % CHUNK_SIZE);
}

// ----------------------------------------------------------------------------

Vector3 ApproximateZeroCrossingPosition(std::shared_ptr<godot::Chunk> chunk, const Vector3& p0, const Vector3& p1)
{
	// approximate the zero crossing by finding the min value along the edge
	float minValue = 100000.f;
	float t = 0.f;
	float currentT = 0.f;
	const int steps = 8;
	const float increment = 1.f / (float)steps;
	while (currentT <= 1.f)
	{
		const Vector3 p = p0 + ((p1 - p0) * currentT);
		const float density = abs(Density_Func(chunk, p));
		if (density < minValue)
		{
			minValue = density;
			t = currentT;
		}

		currentT += increment;
	}

	return p0 + ((p1 - p0) * t);
}

// ----------------------------------------------------------------------------

Vector3 CalculateSurfaceNormal(std::shared_ptr<godot::Chunk> chunk, const Vector3& p)
{
	const float H = 0.001f;
	const float dx = Density_Func(chunk, p + Vector3(H, 0.f, 0.f)) - Density_Func(chunk, p - Vector3(H, 0.f, 0.f));
	const float dy = Density_Func(chunk, p + Vector3(0.f, H, 0.f)) - Density_Func(chunk, p - Vector3(0.f, H, 0.f));
	const float dz = Density_Func(chunk, p + Vector3(0.f, 0.f, H)) - Density_Func(chunk, p - Vector3(0.f, 0.f, H));

	return Vector3(dx, dy, dz).normalized();
}

// ----------------------------------------------------------------------------

std::shared_ptr<OctreeNode> ConstructLeaf(std::shared_ptr<OctreeNode> leaf)
{
	if (!leaf || leaf->size != 1)
	{
		return nullptr;
	}

	int corners = 0;
	for (int i = 0; i < 8; i++)
	{
		const Vector3 cornerPos = leaf->min + CHILD_MIN_OFFSETS[i];
		const float density = Density_Func(leaf->chunk, Vector3(cornerPos));
		const int material = density < 0.f ? MATERIAL_SOLID : MATERIAL_AIR;
		corners |= (material << i);
	}

	if (corners == 0 || corners == 255)
	{
		// voxel is full inside or outside the volume
		leaf.reset();
		return nullptr;
	}

	// otherwise the voxel contains the surface, so find the edge intersections
	const int MAX_CROSSINGS = 6;
	int edgeCount = 0;
	Vector3 averageNormal(0, 0, 0);
	svd::QefSolver qef;

	for (int i = 0; i < 12 && edgeCount < MAX_CROSSINGS; i++)
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

		const Vector3 p1 = Vector3(leaf->min + CHILD_MIN_OFFSETS[c1]);
		const Vector3 p2 = Vector3(leaf->min + CHILD_MIN_OFFSETS[c2]);
		const Vector3 p = ApproximateZeroCrossingPosition(leaf->chunk, p1, p2);
		const Vector3 n = CalculateSurfaceNormal(leaf->chunk, p);
		qef.add(p.x, p.y, p.z, n.x, n.y, n.z);

		averageNormal += n;

		edgeCount++;
	}

	svd::Vec3 qefPosition;
	qef.solve(qefPosition, QEF_ERROR, QEF_SWEEPS, QEF_ERROR);

	std::shared_ptr<OctreeDrawInfo> drawInfo = make_shared<OctreeDrawInfo>();
	drawInfo->position = Vector3(qefPosition.x, qefPosition.y, qefPosition.z);
	drawInfo->qef = qef.getData();

	const Vector3 min = Vector3(leaf->min);
	const Vector3 max = Vector3(leaf->min + Vector3(leaf->size, leaf->size, leaf->size));
	if (drawInfo->position.x < min.x || drawInfo->position.x > max.x ||
		drawInfo->position.y < min.y || drawInfo->position.y > max.y ||
		drawInfo->position.z < min.z || drawInfo->position.z > max.z)
	{
		const auto& mp = qef.getMassPoint();
		drawInfo->position = Vector3(mp.x, mp.y, mp.z);
	}

	drawInfo->averageNormal = (averageNormal / (float)edgeCount).normalized();
	drawInfo->corners = corners;

	leaf->type = Node_Leaf;
	leaf->drawInfo = drawInfo;

	return leaf;
}

// -------------------------------------------------------------------------------

std::shared_ptr<OctreeNode> ConstructOctreeNodes(std::shared_ptr<OctreeNode> node)
{
	if (!node)
	{
		return nullptr;
	}

	if (node->size == 1)
	{
		return ConstructLeaf(node);
	}

	const int childSize = node->size / 2;
	bool hasChildren = false;

	for (int i = 0; i < 8; i++)
	{
		std::shared_ptr<OctreeNode> child = make_shared<OctreeNode>();
		child->size = childSize;
		child->min = node->min + (CHILD_MIN_OFFSETS[i] * childSize);
		child->type = Node_Internal;
		child->chunk = node->chunk;

		node->children[i] = ConstructOctreeNodes(child);
		hasChildren |= (node->children[i] != nullptr);
	}

	if (!hasChildren)
	{
		node.reset();
		return nullptr;
	}

	return node;
}

// -------------------------------------------------------------------------------

std::shared_ptr<OctreeNode> BuildOctree(const Vector3& min, const int size, const float threshold, std::shared_ptr<godot::Chunk> chunk)
{
	std::shared_ptr<OctreeNode> root = make_shared<OctreeNode>();
	root->min = min;
	root->size = size;
	root->type = Node_Internal;
	root->chunk = chunk;
	chunk->setRoot(root);

	ConstructOctreeNodes(root);
	root = SimplifyOctree(root, threshold);

	return root;
}

// ----------------------------------------------------------------------------

void GenerateMeshFromOctree(std::shared_ptr<OctreeNode> node, VertexBuffer& vertexBuffer, IndexBuffer& indexBuffer, int* counts)
{
	if (!node)
	{
		return;
	}

	//vertexBuffer.clear();
	//indexBuffer.clear();

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

vector<std::shared_ptr<OctreeNode>> FindSeamNodes(std::shared_ptr<godot::Chunk> chunk)
{
	auto world = chunk->getWorld();
	const Vector3 baseChunkMin = chunk->getOffset();
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
		auto c = world->getChunk(chunkMin);
		if (c)
		{
			auto chunkNodes = c->findNodes(selectionFuncs[i]);
			seamNodes.insert(end(seamNodes), begin(chunkNodes), end(chunkNodes));
		}
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

	if (node->type == Node_Leaf || node->type == Node_Psuedo)
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

vector<std::shared_ptr<OctreeNode>> BuildSeamOctree(vector<std::shared_ptr<OctreeNode>> seams, std::shared_ptr<godot::Chunk> chunk, int parentSize)
{
	vector<std::shared_ptr<OctreeNode>> parents;

	for (int i = 0; i < seams.size(); i++) {
		std::shared_ptr<OctreeNode> node = seams[i];

		if (node->size * 2 != parentSize) {
			parents.push_back(node);
			continue;
		}

		Vector3 chunkMin = chunk->getOffset();
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

		for (int j = 0; j < parents.size(); j++) {
			if (parents[j]->min == parentMin) {
				hasElement = true;
				parents[j]->children[index] = node;
				break;
			}
		}

		if (!hasElement) {
			std::shared_ptr<OctreeNode> parent = make_shared<OctreeNode>();
			parent->min = parentMin;
			parent->size = parentSize;
			parent->type = Node_Internal;
			parent->chunk = node->chunk;
			parents.push_back(parent);
			parent->children[index] = node;
		}
	}

	if (parents.size() < 2) return parents;
	return BuildSeamOctree(parents, chunk, parentSize * 2);
}

// -------------------------------------------------------------------------------
