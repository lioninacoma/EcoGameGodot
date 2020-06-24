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

void ChunkBuilder::buildChunk(std::shared_ptr<Chunk> chunk) {
	if (!chunk) return;
	Node* parent = world->get_parent();

	bpt::ptime start, stop;
	bpt::time_duration dur;
	long ms = 0;

	start = bpt::microsec_clock::local_time();
	int i, j, n, amountVertices, amountIndices, amountFaces;
	
	VertexBuffer vertices;
	IndexBuffer indices;

	//Godot::print(String("vertices at chunk {0} building ...").format(Array::make(chunk->getOffset())));
	
	BUILD_MESH_MUTEX.lock();
	try {
		OctreeNode* root = BuildOctree(chunk->getOffset(), CHUNK_SIZE_X, 50.f, chunk.get());
		GenerateMeshFromOctree(root, vertices, indices);
	}
	catch (const std::exception & e) {
		//std::cerr << boost::diagnostic_information(e);
		Godot::print(String("chunk at chunk {0} is empty").format(Array::make(chunk->getOffset())));
	}
	BUILD_MESH_MUTEX.unlock();

	if (vertices.size() <= 0) {
		stop = bpt::microsec_clock::local_time();
		dur = stop - start;
		ms = dur.total_milliseconds();

		parent->call_deferred("delete_chunk", chunk.get(), world.get());

		if (chunk->isNavigatable())
			Godot::print(String("chunk at {0} deleted").format(Array::make(chunk->getOffset(), ms)));
		chunk->setNavigatable(false);
		chunk->setBuilding(false);
		BUILD_QUEUE_CV.notify_one();
		return;
	}

	Array meshData;
	Array meshArrays;
	unordered_map<size_t, Vector3> nodePoints;

	amountVertices = vertices.size();
	amountIndices = indices.size();
	amountFaces = amountIndices / 3;

	Godot::print(String("amountVertices: {0}, amountFaces: {1}").format(Array::make(amountVertices, amountFaces)));

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
		vertexArrayWrite[i] = Vector3(vertices[i].xyz.x, vertices[i].xyz.y, vertices[i].xyz.z);
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

	parent->call_deferred("build_chunk", meshData, chunk.get(), world.get());

	stop = bpt::microsec_clock::local_time();
	dur = stop - start;
	ms = dur.total_milliseconds();

	Godot::print(String("chunk at {0} built in {1} ms").format(Array::make(chunk->getOffset(), ms)));
	chunk->setNavigatable(true);
	chunk->setBuilding(false);
	BUILD_QUEUE_CV.notify_one();
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
	ThreadPool::get()->submitTask(boost::bind(&ChunkBuilder::buildChunk, this, chunk));
}
