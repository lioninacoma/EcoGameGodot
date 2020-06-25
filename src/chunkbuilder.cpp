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

void ChunkBuilder::buildMesh(std::shared_ptr<Chunk> chunk, std::shared_ptr<OctreeNode> root, std::shared_ptr<OctreeNode> seam) {
	Node* parent = world->get_parent();
	int i, j, n, amountVertices, amountIndices, amountFaces;

	VertexBuffer vertices;
	IndexBuffer indices;

	GenerateMeshFromOctree(root, vertices, indices);
	if (seam) {
		GenerateMeshFromOctree(seam, vertices, indices); 
		DestroyOctree(seam);
	}

	amountVertices = vertices.size();
	amountIndices = indices.size();
	amountFaces = amountIndices / 3;

	//Godot::print(String("amountVertices: {0}, amountFaces: {1}").format(Array::make(amountVertices, amountFaces)));

	if (amountVertices <= 0 || amountIndices <= 0) {
		parent->call_deferred("delete_chunk", chunk.get(), world.get());

		if (chunk->isNavigatable())
			Godot::print(String("chunk at {0} deleted").format(Array::make(chunk->getOffset())));
		chunk->setNavigatable(false);
		chunk->setBuilding(false);
		BUILD_QUEUE_CV.notify_one();
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
		vertexArrayWrite[i] = Vector3(vertices[i].xyz);
	}

	for (i = 0, n = 0; i < amountFaces; i++, n += 3) {
		indexArrayWrite[n] = indices[n];
		indexArrayWrite[n + 1] = indices[n + 2];
		indexArrayWrite[n + 2] = indices[n + 1];

		normalArrayWrite[indices[n]] = Vector3(vertices[indices[n]].normal);
		normalArrayWrite[indices[n + 1]] = Vector3(vertices[indices[n + 1]].normal);
		normalArrayWrite[indices[n + 2]] = Vector3(vertices[indices[n + 2]].normal);

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
}

void ChunkBuilder::buildChunk(std::shared_ptr<Chunk> chunk) {
	if (!chunk) return;

	bpt::ptime start, stop;
	bpt::time_duration dur;
	long ms = 0;

	start = bpt::microsec_clock::local_time();

	vector<std::shared_ptr<OctreeNode>> seamNodes;
	std::shared_ptr<OctreeNode> root = NULL, seam = NULL;
	const Vector3 baseChunkMin = chunk->getOffset();
	const Vector3 OFFSETS[8] =
	{
		Vector3(0,0,0), Vector3(1,0,0), Vector3(0,0,1), Vector3(1,0,1),
		Vector3(0,1,0), Vector3(1,1,0), Vector3(0,1,1), Vector3(1,1,1)
	};

	//Godot::print(String("vertices at chunk {0} building ...").format(Array::make(chunk->getOffset())));

	try {
		root = chunk->getRoot();
		seamNodes = FindSeamNodes(chunk);
		seamNodes = BuildSeamOctree(seamNodes, chunk);

		if (seamNodes.size() == 1) {
			seam = seamNodes.front();
		}

		buildMesh(chunk, root, seam);
	}
	catch (const std::exception & e) {
		std::cerr << boost::diagnostic_information(e);
	}

	stop = bpt::microsec_clock::local_time();
	dur = stop - start;
	ms = dur.total_milliseconds();

	Godot::print(String("chunk at {0} built in {1} ms").format(Array::make(baseChunkMin, ms)));
	chunk->setNavigatable(true);
	chunk->setBuilding(false);

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
	ThreadPool::get()->submitTask(boost::bind(&ChunkBuilder::buildChunk, this, chunk));
}
