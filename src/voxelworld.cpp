#include "voxelworld.h"

using namespace godot;

void VoxelWorld::_register_methods() {
	register_method("build", &VoxelWorld::build);
	register_method("update", &VoxelWorld::update);
	register_method("_notification", &VoxelWorld::_notification);
}

void VoxelWorld::_init() {

}

void VoxelWorld::_notification(const int64_t what) {
	if (what == Node::NOTIFICATION_READY) {}
}

void VoxelWorld::build() {
	root = make_shared<OctreeNode>();
	root->min = Vector3();
	root->size = pow(2, MAX_LOD) * CHUNK_SIZE;
	root->type = Node_Internal;
	root->lod = MAX_LOD;

	vector<std::shared_ptr<godot::OctreeNode>> nodes;
	
	ExpandNodes(root, root, nodes);

	for (auto node : nodes) {
		buildTree(node);
	}

	// #draw
	for (auto node : nodes) {
		buildMesh(node);
	}
	
	return;
	int buildList[] = { 1, 3, 7, 2 };

	for (int childIndex : buildList) {
		nodes.clear();
		//ExpandNodes(root, root->children[childIndex], nodes);
		ExpandNodes(root, root, Vector3(), CHUNK_SIZE * 4, nodes);

		sort(nodes.begin(), nodes.end(), [](const std::shared_ptr<OctreeNode>& a, const std::shared_ptr<OctreeNode>& b) {
			return a->min * a->size < b->min * b->size;
		});

		for (auto node : nodes) {
			buildTree(node);
		}

		for (auto node : nodes) {
			buildMesh(node);
		}
	}
}

#define CHUNKBUILDER_VERTEX_SIZE 3
#define CHUNKBUILDER_FACE_SIZE 4
#define CHUNKBUILDER_MAX_VERTICES 640000
#define CHUNKBUILDER_MAX_FACES CHUNKBUILDER_MAX_VERTICES / 3

void VoxelWorld::update(Vector3 camera) {
	//return;
	Node* scene = get_parent();

	if (!buildQueue.empty()) {
		auto next = buildQueue.front();
		buildQueue.pop_front();


		// TODO: check if seam node is correct at side x

		auto neighbours = FindSeamNeighbours(root, next);
		cout << "amount neighbours: " << neighbours.size() << endl;
		for (auto neighbour : neighbours) {
			buildQueue.push_back(neighbour);
		}

		buildTree(next);
		buildMesh(next);
	}
	else {
		cout << "------------- fill queue -------------" << endl;
		auto node = ExpandNode(root, Vector3(), CHUNK_SIZE * 2);

		if (node && node != root) {
			int i;
			std::shared_ptr<OctreeNode> child;

			for (i = 0; i < 8; i++) {
				child = node->children[i];
				if (!child) continue;
				buildQueue.push_front(child);
				child->dirty = true;
			}
		}
	}
}

void VoxelWorld::buildTree(std::shared_ptr<OctreeNode> node) {
	auto leafs = FindActiveVoxels(node);

	if (leafs.empty()) {
		Godot::print(String("empty node min: {0}, size: {1}").format(Array::make(node->min, node->size)));
		return;
	}

	auto meshRoot = BuildOctree(leafs, node->min, 1);
	node->meshRoot = meshRoot;
}

void VoxelWorld::buildMesh(std::shared_ptr<OctreeNode> node) {
	Node* parent = get_parent();
	int i, j, n, amountVertices, amountIndices, amountFaces;

	VertexBuffer vertices;
	IndexBuffer indices;
	int* counts = new int[2]{ 0, 0 };
	vertices.resize(CHUNKBUILDER_MAX_VERTICES);
	indices.resize(CHUNKBUILDER_MAX_VERTICES);

	node->dirty = false;

	GenerateMeshFromOctree(node->meshRoot, vertices, indices, counts);

	if (counts[0] == 0 || counts[1] == 0) return;

	auto seams = FindSeamNodes(root, node);
	if (seams.size() > 0) {
		auto seamRoot = BuildOctree(seams, node->min, 1);
		GenerateMeshFromOctree(seamRoot, vertices, indices, counts);
	}
	
	amountVertices = counts[0];
	amountIndices = counts[1];
	amountFaces = amountIndices / 3;

	node->meshInstanceId = DeleteMesh(node);
	Godot::print(String("amountVertices: {0}, amountIndices: {1}, seams: {2}, meshId: {3}").format(Array::make(amountVertices, amountIndices, seams.size(), node->meshInstanceId)));

	if (counts[0] == 0 || counts[1] == 0) {
		if (node->meshInstanceId) {
			cout << "delete" << endl;
			parent->call("delete_chunk", node.get(), this);
		}
		return;
	}

	Array meshData;
	Array meshArrays;

	PoolVector3Array vertexArray;
	PoolVector3Array normalArray;
	PoolIntArray indexArray;
	PoolVector3Array collisionArray;

	meshData.resize(2);
	meshArrays.resize(Mesh::ARRAY_MAX);
	vertexArray.resize(amountVertices);
	normalArray.resize(amountVertices);
	indexArray.resize(amountIndices);
	collisionArray.resize(amountIndices);

	PoolVector3Array::Write vertexArrayWrite = vertexArray.write();
	PoolVector3Array::Write normalArrayWrite = normalArray.write();
	PoolIntArray::Write indexArrayWrite = indexArray.write();
	PoolVector3Array::Write collisionArrayWrite = collisionArray.write();

	for (i = 0; i < amountVertices; i++) {
		vertexArrayWrite[i] = vertices[i].xyz;
	}

	for (i = 0, n = 0; i < amountFaces; i++, n += 3) {
		indexArrayWrite[n] = indices[n];
		indexArrayWrite[n + 1] = indices[n + 2];
		indexArrayWrite[n + 2] = indices[n + 1];

		normalArrayWrite[indices[n]] = vertices[indices[n]].normal;
		normalArrayWrite[indices[n + 1]] = vertices[indices[n + 1]].normal;
		normalArrayWrite[indices[n + 2]] = vertices[indices[n + 2]].normal;

		collisionArrayWrite[n] = vertexArrayWrite[indices[n]];
		collisionArrayWrite[n + 1] = vertexArrayWrite[indices[n + 1]];
		collisionArrayWrite[n + 2] = vertexArrayWrite[indices[n + 2]];
	}

	meshArrays[Mesh::ARRAY_VERTEX] = vertexArray;
	meshArrays[Mesh::ARRAY_NORMAL] = normalArray;
	meshArrays[Mesh::ARRAY_INDEX] = indexArray;

	meshData[0] = meshArrays;
	meshData[1] = collisionArray;

	parent->call("build_chunk", meshData, node.get(), this);
}