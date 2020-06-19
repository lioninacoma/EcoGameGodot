#include "chunkbuilder.h"
#include "ecogame.h"
#include "voxelworld.h"
#include "threadpool.h"
#include "navigator.h"
#include "graphnode.h"

#include <boost/exception/diagnostic_information.hpp> 
#include <boost/exception_ptr.hpp> 

using namespace godot;

ChunkBuilder::ChunkBuilder(std::shared_ptr<VoxelWorld> world) {
	ChunkBuilder::world = world;
	ChunkBuilder::meshBuilder = std::make_unique<MeshBuilder>(world);
	ChunkBuilder::threadStarted = false;
}

//#define DEBUG_NODE_COUNT
//#define DEBUG_FACE_COUNT

void ChunkBuilder::buildChunk(std::shared_ptr<Chunk> chunk) {
	if (!chunk) return;
	Node* parent = world->get_parent();

	bpt::ptime start, stop;
	bpt::time_duration dur;
	long ms = 0;

	start = bpt::microsec_clock::local_time();
	int i, j, n, amountVertices, amountIndices, amountFaces;

	float** vertices = new float* [CHUNKBUILDER_MAX_VERTICES];
	for (i = 0; i < CHUNKBUILDER_MAX_VERTICES; i++) {
		vertices[i] = new float[CHUNKBUILDER_VERTEX_SIZE];
		memset(vertices[i], 0, CHUNKBUILDER_VERTEX_SIZE * sizeof(float));
	}

	int** faces = new int* [CHUNKBUILDER_MAX_FACES];
	for (i = 0; i < CHUNKBUILDER_MAX_FACES; i++) {
		faces[i] = new int[CHUNKBUILDER_FACE_SIZE];
		memset(faces[i], 0, CHUNKBUILDER_FACE_SIZE * sizeof(int));
	}
	
	try {
		//Godot::print(String("vertices at chunk {0} building ...").format(Array::make(chunk->getOffset())));
		BUILD_MESH_MUTEX.lock();
		int* counts = meshBuilder->buildVertices(chunk, vertices, faces);
		BUILD_MESH_MUTEX.unlock();

		if (counts[0] <= 0) {
			for (i = 0; i < CHUNKBUILDER_MAX_VERTICES; i++)
				delete[] vertices[i];
			for (i = 0; i < CHUNKBUILDER_MAX_FACES; i++)
				delete[] faces[i];
			delete[] vertices;
			delete[] faces;

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

		amountVertices = counts[0];
		amountFaces = counts[1];
		amountIndices = amountFaces * 3;

#ifdef DEBUG_FACE_COUNT
		amountFacesTotal += amountFaces;
		if (amountFacesTotal % 4 == 0) {
			Godot::print(String("{0} has {1} faces").format(Array::make(world->get_name(), amountFacesTotal)));
		}
#endif //DEBUG_FACE_COUNT

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

		if (chunk->isNavigatable()) {
			std::function<void(pair<size_t, std::shared_ptr<GraphNode>>)> lambda = [&](auto next) {
				nodePoints.emplace(next.first, next.second->getPointU());
			};
			chunk->forEachNode(lambda);
		}
		
		for (i = 0; i < amountVertices; i++) {
			vertexArrayWrite[i] = Vector3(vertices[i][0], vertices[i][1], vertices[i][2]);
			normalArrayWrite[i] = Vector3(0, 0, 0);
			if (chunk->isNavigatable())
				nodePoints.erase(fn::hash(vertexArray[i]));
		}

		for (auto nodePoint : nodePoints) {
			auto node = chunk->getNode(fn::hash(nodePoint.second));
			if (!node) continue;
			chunk->removeNode(node);
		}

		for (i = 0, n = 0; i < amountFaces; i++, n += 3) {
			indexArrayWrite[n] = faces[i][2];
			indexArrayWrite[n + 1] = faces[i][1];
			indexArrayWrite[n + 2] = faces[i][0];

			Vector3 x0 = vertexArray[faces[i][0]];
			Vector3 x1 = vertexArray[faces[i][1]];
			Vector3 x2 = vertexArray[faces[i][2]];
			Vector3 v0 = x0 - x2;
			Vector3 v1 = x1 - x2;
			Vector3 normal = v0.cross(v1).normalized();

			normalArrayWrite[faces[i][0]] = fn::addNormal(normalArrayWrite[faces[i][0]], normal);
			normalArrayWrite[faces[i][1]] = fn::addNormal(normalArrayWrite[faces[i][1]], normal);
			normalArrayWrite[faces[i][2]] = fn::addNormal(normalArrayWrite[faces[i][2]], normal);

			collisionArrayWrite[n] = vertexArray[faces[i][0]];
			collisionArrayWrite[n + 1] = vertexArray[faces[i][1]];
			collisionArrayWrite[n + 2] = vertexArray[faces[i][2]];
			
			//chunk->addFaceNodes(x0, x1, x2, normal);
		}

#ifdef DEBUG_NODE_COUNT
		size_t chunkHash = fn::hash(chunk->getOffset());
		int amountNodes = chunk->getAmountNodes();
		int amountEdges = 0;
		std::function<void(pair<size_t, std::shared_ptr<GraphNode>>)> cntFn = [&](auto next) {
			amountEdges += next.second->getAmountEdges();
		};
		chunk->forEachNode(cntFn);
		nodeCounts[chunkHash] = amountNodes;
		edgeCounts[chunkHash] = amountEdges;
		int nodeTotalCount = 0, edgeTotalCount = 0;
		for (auto cnt : nodeCounts)
			nodeTotalCount += cnt.second;
		for (auto cnt : edgeCounts)
			edgeTotalCount += cnt.second;
		Godot::print(String("total nodes {0}, total edges {1}").format(Array::make(nodeTotalCount, edgeTotalCount)));
#endif //DEBUG_NODE_COUNT

		for (i = 0; i < CHUNKBUILDER_MAX_VERTICES; i++)
			delete[] vertices[i];
		for (i = 0; i < CHUNKBUILDER_MAX_FACES; i++)
			delete[] faces[i];
		delete[] vertices;
		delete[] faces;
 
		meshArrays[Mesh::ARRAY_VERTEX] = vertexArray;
		meshArrays[Mesh::ARRAY_NORMAL] = normalArray;
		meshArrays[Mesh::ARRAY_INDEX] = indexArray;
		
		meshData[0] = meshArrays;
		meshData[1] = collisionArray;
		
		parent->call_deferred("build_chunk", meshData, chunk.get(), world.get());
	}
	catch (const std::exception & e) {
		std::cerr << boost::diagnostic_information(e);
	}

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
