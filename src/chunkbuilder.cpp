#include "chunkbuilder.h"
#include "ecogame.h"
#include "voxelworld.h"
#include "threadpool.h"
#include "navigator.h"
#include "graphnode.h"
#include "octree.h"

using namespace godot;

ChunkBuilder::ChunkBuilder(std::shared_ptr<VoxelWorld> world) {
	ChunkBuilder::world = world;
	ChunkBuilder::meshBuilder = std::make_unique<MeshBuilder>(world);
	ChunkBuilder::threadStarted = false;
}

void ChunkBuilder::buildMesh(std::shared_ptr<Chunk> chunk) {
	Node* parent = world->get_parent();
	int i, j, n, amountVertices, amountIndices, amountFaces;

	const Vector3 chunkMin = chunk->getOffset();
	const Vector3 OFFSETS[7] =
	{
						Vector3(1,0,0), Vector3(0,0,1), Vector3(1,0,1),
		Vector3(0,1,0), Vector3(1,1,0), Vector3(0,1,1), Vector3(1,1,1)
	};
	
	VertexBuffer vertices;
	IndexBuffer indices;
	int* counts = new int[2] {0, 0};
	vertices.resize(CHUNKBUILDER_MAX_VERTICES);
	indices.resize(CHUNKBUILDER_MAX_VERTICES);

	//auto leafs = BuildMesh(chunkMin, CHUNK_SIZE, vertices, indices, counts);

	//if (!leafs.empty()) {
	//	leafs = BuildOctree(leafs, chunkMin, 1);
	//	
	//	if (leafs.size() == 1) {
	//		auto root = leafs.front();
	//		chunk->setRoot(root);

	//		auto seams = FindSeamNodes(world, chunkMin);
	//		seams = BuildOctree(seams, chunkMin, 1);
	//		if (seams.size() == 1) {
	//			auto seamRoot = seams.front();
	//			GenerateMeshFromOctree(seamRoot, vertices, indices, counts);
	//		}
	//	}
	//}

	auto leafs = BuildMesh(chunkMin, CHUNK_SIZE, vertices, indices, counts);

	if (leafs.empty()) {
		chunk->empty = true;
		return;
	}

	leafs = BuildOctree(leafs, chunkMin, 1);
	auto root = leafs.front();

	//vertices.clear();
	//indices.clear();
	//counts[0] = 0; counts[1] = 0;
	//GenerateMeshFromOctree(root, vertices, indices, counts);

	root = SimplifyOctree(root, 1.f);
	chunk->setRoot(root);

	for (int i = 0; i < 7; i++) {
		const Vector3 offsetMin = OFFSETS[i] * CHUNK_SIZE;
		const Vector3 neighbourChunkMin = chunkMin + offsetMin;
		auto neighbour = world->getChunk(neighbourChunkMin);
		if (!neighbour || neighbour->empty) continue;
		if (!neighbour->getRoot()) {
			addWaiting(fn::hash(neighbourChunkMin), chunk);
			return;
		}
	}

	auto seams = FindSeamNodes(world, chunkMin);
	seams = BuildOctree(seams, chunkMin, 1);
	if (seams.size() == 1) {
		auto seamRoot = seams.front();
		GenerateMeshFromOctree(seamRoot, vertices, indices, counts);
		DestroyOctree(seamRoot);
	}

	//auto root = chunk->getRoot();
	//GenerateMeshFromOctree(root, vertices, indices, counts);

	//auto seams = FindSeamNodes(world, chunkMin);
	//seams = BuildOctree(seams, chunkMin, 1);

	//if (seams.size() == 1) {
	//	auto seamRoot = seams.front();
	//	GenerateMeshFromOctree(seamRoot, vertices, indices, counts);
	//	DestroyOctree(seamRoot);
	//}

	//GenerateMeshFromOctree(chunk->getRoot(), vertices, indices, counts);
	//if (seam) {
	//	GenerateMeshFromOctree(seam, vertices, indices, counts);
	//	DestroyOctree(seam);
	//}

	amountVertices = counts[0];
	amountIndices = counts[1];
	amountFaces = amountIndices / 3;

	//Godot::print(String("amountVertices: {0}, amountFaces: {1}").format(Array::make(amountVertices, amountFaces)));

	if (amountVertices <= 0 || amountIndices <= 0) {
		if (chunk->getMeshInstanceId() > 0)
			parent->call_deferred("delete_chunk", chunk.get(), world.get());
		return;
	}

	Array meshData;
	Array meshArrays;
	unordered_map<size_t, Vector3> nodePoints;

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

		collisionArrayWrite[n] = vertexArray[indices[n]];
		collisionArrayWrite[n + 1] = vertexArray[indices[n + 1]];
		collisionArrayWrite[n + 2] = vertexArray[indices[n + 2]];
	}

	meshArrays[Mesh::ARRAY_VERTEX] = vertexArray;
	meshArrays[Mesh::ARRAY_NORMAL] = normalArray;
	meshArrays[Mesh::ARRAY_INDEX] = indexArray;

	meshData[0] = meshArrays;
	meshData[1] = collisionArray;

	//MeshInstance* meshInstance = MeshInstance::_new();
	//ArrayMesh* mesh = ArrayMesh::_new();
	//StaticBody staticBody;

	//mesh->add_surface_from_arrays(ArrayMesh::PRIMITIVE_TRIANGLES, meshData[0]);
	//mesh.surface_set_material(0, material);

	/*var polygon_shape : ConcavePolygonShape = ConcavePolygonShape.new()
	polygon_shape.set_faces(mesh_data[1])
	var owner_id = staticBody.create_shape_owner(owner)
	staticBody.shape_owner_add_shape(owner_id, polygon_shape)*/

	//meshInstance.add_child(staticBody)
	//meshInstance->set_mesh(mesh);
	//world->call_deferred("add_child", meshInstance);
	
	parent->call_deferred("build_chunk", meshData, chunk.get(), world.get());
}

void ChunkBuilder::buildChunk(std::shared_ptr<Chunk> chunk) {
	if (!chunk) return;

	bpt::ptime start, stop;
	bpt::time_duration dur;
	long ms = 0;

	start = bpt::microsec_clock::local_time();

	const Vector3 chunkMin = chunk->getOffset();

	//Godot::print(String("building chunk at {0}").format(Array::make(chunkMin)));

	try {
		buildMesh(chunk);
	}
	catch (const std::exception & e) {
		std::cerr << boost::diagnostic_information(e);
	}

	stop = bpt::microsec_clock::local_time();
	dur = stop - start;
	ms = dur.total_milliseconds();

	Godot::print(String("chunk at {0} built in {1} ms").format(Array::make(chunkMin, ms)));
	chunk->setNavigatable(true);
	chunk->setBuilding(false);

	queueWaiting(fn::hash(chunkMin));

	BUILD_QUEUE_CV.notify_one();
}

void ChunkBuilder::addWaiting(size_t notifying, std::shared_ptr<Chunk> chunk) {
	auto it = waitingQueue.find(notifying);
	unordered_set<std::shared_ptr<Chunk>>* waitingList;
	
	if (it == waitingQueue.end()) {
		waitingList = new unordered_set<std::shared_ptr<Chunk>>();
		waitingQueue[notifying] = waitingList;
	}
	else {
		waitingList = it->second;
	}

	waitingList->insert(chunk);
}

void ChunkBuilder::queueWaiting(size_t notifying) {
	auto it = waitingQueue.find(notifying);
	if (it == waitingQueue.end()) return;

	auto waitingList = it->second;
	for (auto waiting : *waitingList) {
		//Godot::print(String("queue chunk {0}").format(Array::make(waiting->getOffset())));
		queueChunk(waiting);
	}

	waitingList->clear();
}

void ChunkBuilder::processQueue() {
	while (true) {
		boost::unique_lock<boost::mutex> lock(BUILD_QUEUE_WAIT);
		BUILD_QUEUE_CV.wait(lock);
		if (buildQueue.empty()) continue;

		BUILD_QUEUE_MUTEX.lock();
		std::shared_ptr<Chunk> chunk = buildQueue.front();
		buildQueue.pop_front();
		size_t hash = fn::hash(chunk->getOffset());
		inque.erase(hash);
		BUILD_QUEUE_MUTEX.unlock();

		build(chunk);
	}
}

void ChunkBuilder::queueChunk(std::shared_ptr<Chunk> chunk) {
	boost::unique_lock<boost::mutex> lock(BUILD_QUEUE_MUTEX);

	size_t hash = fn::hash(chunk->getOffset());
	if (inque.find(hash) != inque.end()) return;
	inque.insert(hash);
	buildQueue.push_back(chunk);
}

void ChunkBuilder::build(std::shared_ptr<Chunk> chunk) {
	if (!threadStarted) {
		threadStarted = true;
		queueThread = std::make_unique<boost::thread>(&ChunkBuilder::processQueue, this);
	}

	if (chunk->isBuilding()) {
		queueChunk(chunk);
		return;
	}

	chunk->setBuilding(true);
	//ThreadPool::get()->submitTask(boost::bind(&ChunkBuilder::buildChunk, this, chunk));
	buildChunk(chunk);
}
