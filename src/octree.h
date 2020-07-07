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


#ifndef		HAS_OCTREE_H_BEEN_INCLUDED
#define		HAS_OCTREE_H_BEEN_INCLUDED

#include <Vector3.hpp>

#include <vector>

#include <functional>
#include <boost/function.hpp>

#include "constants.h"
#include "qef.h"
#include "voxeldata.h"

using namespace std;

// ----------------------------------------------------------------------------

const godot::Vector3 CHILD_MIN_OFFSETS[] = {
	// needs to match the vertMap from Dual Contouring impl
	godot::Vector3(0, 0, 0),
	godot::Vector3(0, 0, 1),
	godot::Vector3(0, 1, 0),
	godot::Vector3(0, 1, 1),
	godot::Vector3(1, 0, 0),
	godot::Vector3(1, 0, 1),
	godot::Vector3(1, 1, 0),
	godot::Vector3(1, 1, 1),
};

// ----------------------------------------------------------------------------

struct MeshVertex {
	MeshVertex() : MeshVertex(godot::Vector3(), godot::Vector3()) {}
	MeshVertex(const godot::Vector3& _xyz, const godot::Vector3& _normal) : xyz(_xyz), normal(_normal) {}
	godot::Vector3 xyz, normal;
};

typedef std::vector<MeshVertex> VertexBuffer;
typedef std::vector<int> IndexBuffer;
typedef std::function<bool(const godot::Vector3&, const godot::Vector3&)> FilterNodesFunc;

// ----------------------------------------------------------------------------

enum OctreeNodeType {
	Node_None,
	Node_Internal,
	Node_Pseudo,
	Node_Leaf
};

// ----------------------------------------------------------------------------

struct OctreeChunkInfo {
	OctreeChunkInfo() : OctreeChunkInfo(CHUNK_SIZE) {}
	OctreeChunkInfo(int chunkSize) : chunkSize(chunkSize), volume(make_unique<godot::VoxelData>(chunkSize, chunkSize, chunkSize)) {}
	int chunkSize;
	unique_ptr<godot::VoxelData> volume;

	std::shared_ptr<OctreeChunkInfo> cpy() {
		auto c = make_shared<OctreeChunkInfo>(chunkSize);
		c->chunkSize = chunkSize;
		return c;
	}
};

// ----------------------------------------------------------------------------

struct OctreeDrawInfo {
	OctreeDrawInfo(): index(-1), corners(0) {}
	int				index;
	int				corners;
	godot::Vector3	position;
	godot::Vector3	averageNormal;
	svd::QefData	qef;

	std::shared_ptr<OctreeDrawInfo> cpy() {
		auto c = make_shared<OctreeDrawInfo>();
		c->index = index;
		c->corners = corners;
		c->position = position;
		c->averageNormal = averageNormal;
		c->qef = svd::QefData(qef);
		return c;
	}
};

// ----------------------------------------------------------------------------

class OctreeNode {
public:
	OctreeNode(): OctreeNode(Node_None) {}
	OctreeNode(const OctreeNodeType _type): type(_type), min(0, 0, 0), size(0), drawInfo(nullptr), lod(-1)
		//, chunkInfo(nullptr) 
	{
		
		for (int i = 0; i < 8; i++) {
			children[i] = nullptr;
		}
	}

	~OctreeNode() {

	}

	std::shared_ptr<OctreeNode> cpy() {
		auto c = make_shared<OctreeNode>();
		c->type = type;
		c->min = min;
		c->size = size;
		c->drawInfo = (drawInfo) ? drawInfo->cpy() : nullptr;
		//c->chunkInfo = (chunkInfo) ? chunkInfo->cpy() : nullptr;
		return c;
	}

	int									size;
	int									lod;
	OctreeNodeType						type;
	godot::Vector3						min;
	std::shared_ptr<OctreeNode>			children[8];
	std::shared_ptr<OctreeDrawInfo>		drawInfo;
	//std::shared_ptr<OctreeChunkInfo>	chunkInfo;
};

// ----------------------------------------------------------------------------

namespace godot {
	class VoxelWorld;
}

std::shared_ptr<OctreeNode> SimplifyOctree(std::shared_ptr<OctreeNode> node, float threshold);
void DestroyOctree(std::shared_ptr<OctreeNode> node);
void GenerateMeshFromOctree(std::shared_ptr<OctreeNode> node, VertexBuffer& vertexBuffer, IndexBuffer& indexBuffer, int* counts);
void Octree_FindNodes(std::shared_ptr<OctreeNode> node, FilterNodesFunc& func, vector<std::shared_ptr<OctreeNode>>& nodes);
std::shared_ptr<OctreeNode> BuildOctree(vector<std::shared_ptr<OctreeNode>> seams, godot::Vector3 chunkMin, int parentSize);
void ExpandNodes(std::shared_ptr<OctreeNode> node, int lod);
vector<std::shared_ptr<OctreeNode>> FindSeamNodes(std::shared_ptr<godot::VoxelWorld> world, godot::Vector3 chunkMin);
vector<std::shared_ptr<OctreeNode>> FindLodNodes(std::shared_ptr<OctreeNode> node);
vector<std::shared_ptr<OctreeNode>> FindActiveVoxels(std::shared_ptr<OctreeNode> node);

// ----------------------------------------------------------------------------

#endif	// HAS_OCTREE_H_BEEN_INCLUDED

