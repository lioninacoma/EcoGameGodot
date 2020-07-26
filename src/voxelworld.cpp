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

	return;
	ExpandNodes(root, 0);

	auto lodNodes = FindLodNodes(root);

	for (auto node : lodNodes) {
		if (!node->dirty) continue;
		//Godot::print(String("building node: {0}").format(Array::make(node->min)));
		//continue;
		buildMesh(node);
	}
}

#define CHUNKBUILDER_VERTEX_SIZE 3
#define CHUNKBUILDER_FACE_SIZE 4
#define CHUNKBUILDER_MAX_VERTICES 64000
#define CHUNKBUILDER_MAX_FACES CHUNKBUILDER_MAX_VERTICES / 3

void VoxelWorld::update(Vector3 camera) {
	//return;
	ExpandNodes(root, camera, root->size / 3);
	auto lodNodes = FindLodNodes(root);

	for (auto node : lodNodes) {
		if (!node->dirty) continue;
		//Godot::print(String("building node: {0}").format(Array::make(node->min)));
		//continue;
		buildMesh(node);
	}
}

void VoxelWorld::buildMesh(std::shared_ptr<OctreeNode> node) {
	Node* parent = get_parent();
	int i, j, n, amountVertices, amountIndices, amountFaces;

	node->dirty = false;

	/*const Vector3 min = node->min;
	const Vector3 OFFSETS[7] =
	{
						Vector3(1,0,0), Vector3(0,0,1), Vector3(1,0,1),
		Vector3(0,1,0), Vector3(1,1,0), Vector3(0,1,1), Vector3(1,1,1)
	};*/

	auto leafs = FindActiveVoxels(node);

	cout << leafs.size() << endl;

	if (leafs.empty()) {
		return;
	}

	//leafs.push_back(node);
	auto r = BuildOctree(leafs, node->min, 1);

	/*cout << ((r == node) ? "TRUE" : "FALSE") << endl;
	PrintOctree(r);
	cout << "------------------------------------------" << endl;
	return;*/

	VertexBuffer vertices;
	IndexBuffer indices;
	int* counts = new int[2]{ 0, 0 };
	vertices.resize(CHUNKBUILDER_MAX_VERTICES);
	indices.resize(CHUNKBUILDER_MAX_VERTICES);

	GenerateMeshFromOctree(r, vertices, indices, counts);

	auto seams = FindSeamNodes(root, r);
	//auto seamRoot = BuildOctree(seams, node->min, 1);

	/*auto seams = FindSeamNodes(self, min);
	seams = BuildOctree(seams, min, 1);

	if (seams.size() == 1) {
		auto seamRoot = seams.front();
		GenerateMeshFromOctree(seamRoot, vertices, indices, counts);
		DestroyOctree(seamRoot);
	}*/

	amountVertices = counts[0];
	amountIndices = counts[1];
	amountFaces = amountIndices / 3;

	Godot::print(String("amountVertices: {0}, amountIndices: {1}, seams: {2}").format(Array::make(amountVertices, amountIndices, seams.size())));
	node->meshInstanceId = DeleteMesh(node);

	if (amountVertices == 0 || amountIndices == 0) {
		parent->call_deferred("delete_chunk", this, node.get());
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

	parent->call_deferred("build_chunk", meshData, this, node.get());
}