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

	for (auto node : nodes) {
		buildChunk(node);
		buildSeams(node);
	}
	
	queueThread = std::make_unique<boost::thread>(&VoxelWorld::updateMesh, this);
}

#define CHUNKBUILDER_VERTEX_SIZE 3
#define CHUNKBUILDER_FACE_SIZE 4
#define CHUNKBUILDER_MAX_VERTICES 640000
#define CHUNKBUILDER_MAX_FACES CHUNKBUILDER_MAX_VERTICES / 3

void VoxelWorld::update(Vector3 camera) {
	CAMERA_POS_MUTEX.lock();
	cameraPosition = camera;
	CAMERA_POS_MUTEX.unlock();
	BUILD_QUEUE_CV.notify_one();
}

void VoxelWorld::updateMesh() {
	//return;
	Node* scene = get_parent();

	while (true) {

		if (!buildQueue.empty()) {
			auto next = buildQueue.front();
			buildQueue.pop_front();

			if (next.seams) {
				buildSeams(next.node);
			}
			else if (buildTree(next.node)) {
				buildQueue.push_back(BuildQueueEntry(next.node, true));

				auto neighbours = FindSeamNeighbours(root, next.node);
				for (auto neighbour : neighbours) {
					buildQueue.push_back(BuildQueueEntry(neighbour, true));
				}

				buildChunk(next.node);
			}
		}
		else {
			CAMERA_POS_MUTEX.lock();
			Vector3 cam = cameraPosition;
			CAMERA_POS_MUTEX.unlock();
			auto node = ExpandNode(root, cam, CHUNK_SIZE * 2);

			if (node && node != root) {
				int i;
				std::shared_ptr<OctreeNode> child;

				for (i = 0; i < 8; i++) {
					child = node->children[i];
					if (!child) continue;
					buildQueue.push_front(BuildQueueEntry(child, false));
					child->dirty = true;
				}
			}
			else {
				boost::unique_lock<boost::mutex> lock(BUILD_QUEUE_WAIT);
				BUILD_QUEUE_CV.wait(lock);
			}
		}

	}
}

bool VoxelWorld::buildTree(std::shared_ptr<OctreeNode> node) {
	if (node->hidden) return false;

	auto leafs = FindActiveVoxels(node);
	if (leafs.empty()) return false;

	auto meshRoot = BuildOctree(leafs, node->min, 1);

	if (node->meshRoot) {
		DestroyOctree(node->meshRoot);
	}

	node->meshRoot = meshRoot;
	return true;
}

void VoxelWorld::buildSeams(std::shared_ptr<OctreeNode> node) {
	Node* parent = get_parent();
	VertexBuffer vertices;
	IndexBuffer indices;
	int* counts = new int[2]{ 0, 0 };
	vertices.resize(CHUNKBUILDER_MAX_VERTICES);
	indices.resize(CHUNKBUILDER_MAX_VERTICES);

	auto seams = FindSeamNodes(root, node);
	if (seams.size() > 0) {
		auto seamRoot = BuildOctree(seams, node->min, 1);
		GenerateMeshFromOctree(seamRoot, vertices, indices, counts);
	}

	if (counts[0] == 0 || counts[1] == 0) return;
	cout << "seams - vertices: " << counts[0] << ", indices: " << counts[1] << endl;
	auto meshData = buildMesh(node, vertices, indices, counts);
	parent->call_deferred("build_seams", meshData, node.get(), this);
}

void VoxelWorld::buildChunk(std::shared_ptr<OctreeNode> node) {
	Node* parent = get_parent();
	VertexBuffer vertices;
	IndexBuffer indices;
	int* counts = new int[2]{ 0, 0 };
	vertices.resize(CHUNKBUILDER_MAX_VERTICES);
	indices.resize(CHUNKBUILDER_MAX_VERTICES);

	node->dirty = false;
	GenerateMeshFromOctree(node->meshRoot, vertices, indices, counts);

	if (counts[0] == 0 || counts[1] == 0) return;
	cout << "chunk - vertices: " << counts[0] << ", indices: " << counts[1] << endl;
	auto meshData = buildMesh(node, vertices, indices, counts);
	parent->call_deferred("build_chunk", meshData, node.get(), this);
}

Array VoxelWorld::buildMesh(std::shared_ptr<OctreeNode> node, VertexBuffer vertices, IndexBuffer indices, int* counts) {
	Node* parent = get_parent();
	int i, j, n, amountVertices, amountIndices, amountFaces;
	Array meshData;

	amountVertices = counts[0];
	amountIndices = counts[1];
	amountFaces = amountIndices / 3;

	HideParentMesh(node, this);

	if (counts[0] == 0 || counts[1] == 0) {
		if (!node->meshInstancePath.is_empty()) {
			cout << "delete" << endl;
			parent->call_deferred("delete_chunk", node.get(), this);
		}
		return meshData;
	}

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

	return meshData;
}