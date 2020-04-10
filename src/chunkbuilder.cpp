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

Vector3 addNormal(Vector3 src, Vector3 normal) {
	return (src + normal).normalized();
}

void ChunkBuilder::buildChunk(std::shared_ptr<Chunk> chunk) {
	if (!chunk) return;
	Node* game = EcoGame::get()->getNode();

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
		//if (true) {
			for (i = 0; i < CHUNKBUILDER_MAX_VERTICES; i++)
				delete[] vertices[i];
			for (i = 0; i < CHUNKBUILDER_MAX_FACES; i++)
				delete[] faces[i];
			delete[] vertices;
			delete[] faces;

			stop = bpt::microsec_clock::local_time();
			dur = stop - start;
			ms = dur.total_milliseconds();

			Godot::print(String("chunk at {0} built in {1} ms").format(Array::make(chunk->getOffset(), ms)));
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

		float** chunkVertices = new float* [amountVertices];
		int** chunkFaces = new int* [amountFaces];

		for (i = 0; i < amountVertices; i++) {
			chunkVertices[i] = new float[CHUNKBUILDER_VERTEX_SIZE];
			memcpy(chunkVertices[i], vertices[i], CHUNKBUILDER_VERTEX_SIZE * sizeof(float));
		}

		for (i = 0; i < amountFaces; i++) {
			chunkFaces[i] = new int[CHUNKBUILDER_FACE_SIZE];
			memcpy(chunkFaces[i], faces[i], CHUNKBUILDER_FACE_SIZE * sizeof(int));
		}

		chunk->setVertices(chunkVertices, amountVertices);
		chunk->setFaces(chunkFaces, amountFaces);
		
		PoolVector3Array vertexArray;
		PoolVector3Array normalArray;
		//PoolVector2Array uvArray;
		PoolIntArray indexArray;
		PoolVector3Array collisionArray;

		meshData.resize(2);
		meshArrays.resize(Mesh::ARRAY_MAX);
		vertexArray.resize(amountVertices);
		normalArray.resize(amountVertices);
		//uvArray.resize(amountVertices);
		indexArray.resize(amountIndices);
		collisionArray.resize(amountIndices);

		PoolVector3Array::Write vertexArrayWrite = vertexArray.write();
		PoolVector3Array::Write normalArrayWrite = normalArray.write();
		//PoolVector2Array::Write uvArrayWrite = uvArray.write();
		PoolIntArray::Write indexArrayWrite = indexArray.write();
		PoolVector3Array::Write collisionArrayWrite = collisionArray.write();

		/*const bool DEBUG_NODES_PRINT = false;
		if (DEBUG_NODES_PRINT && chunk->getOffset() == Vector3(32, 0, 32)) {
			int amountEdges = 0;
			std::function<void(pair<size_t, std::shared_ptr<GraphNavNode>>)> lambda = [&](auto next) {
				amountEdges += next.second->getAmountEdges();
			};
			chunk->forEachNode(lambda);
			Godot::print(String("chunk at {0} amount nodes before {1} / amount edges before {2}").format(Array::make(chunk->getOffset(), chunk->getAmountNodes(), amountEdges)));
		}*/

		std::function<void(pair<size_t, std::shared_ptr<GraphNavNode>>)> lambda = [&](auto next) {
			nodePoints.emplace(next.first, next.second->getPointU());
		};
		chunk->forEachNode(lambda);

		for (i = 0; i < amountVertices; i++) {
			//Godot::print(String("v {0}").format(Array::make(Vector3(v[0], v[1], v[2]))));
			vertexArrayWrite[i] = Vector3(vertices[i][0], vertices[i][1], vertices[i][2]);
			normalArrayWrite[i] = Vector3(0, 0, 0);
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

			Vector3 x0 = vertexArray[faces[i][2]];
			Vector3 x1 = vertexArray[faces[i][1]];
			Vector3 x2 = vertexArray[faces[i][0]];
			Vector3 v0 = x0 - x2;
			Vector3 v1 = x1 - x2;
			Vector3 normal = v1.cross(v0).normalized();

			normalArrayWrite[faces[i][0]] = addNormal(normalArrayWrite[faces[i][0]], normal);
			normalArrayWrite[faces[i][1]] = addNormal(normalArrayWrite[faces[i][1]], normal);
			normalArrayWrite[faces[i][2]] = addNormal(normalArrayWrite[faces[i][2]], normal);

			collisionArrayWrite[n] = vertexArray[faces[i][2]];
			collisionArrayWrite[n + 1] = vertexArray[faces[i][1]];
			collisionArrayWrite[n + 2] = vertexArray[faces[i][0]];
			
			//if (!chunk->isNavigatable())
			chunk->addFaceNodes(x0, x1, x2);
		}

		size_t chunkHash = fn::hash(chunk->getOffset());
		int amountNodes = chunk->getAmountNodes();
		int amountEdges = 0;
		std::function<void(pair<size_t, std::shared_ptr<GraphNavNode>>)> cntFn = [&](auto next) {
			amountEdges += next.second->getAmountEdges();
		};
		chunk->forEachNode(cntFn);

		if (!chunk->isNavigatable()) {
			nodeInitialCounts[chunkHash] = amountNodes;
			edgeInitialCounts[chunkHash] = amountEdges;
			nodeInitialTotalCount += amountNodes;
			edgeInitialTotalCount += amountEdges;
		}
		else {
			int deltaAmountNodes = amountNodes - nodeInitialCounts[chunkHash];
			int deltaAmountEdges = amountEdges - edgeInitialCounts[chunkHash];
			nodeInitialTotalCount += deltaAmountNodes;
			edgeInitialTotalCount += deltaAmountEdges;
			//Godot::print(String("chunk at {0}: delta nodes {1}, delta edges {2}").format(Array::make(chunk->getOffset(), deltaAmountNodes, deltaAmountEdges)));
			Godot::print(String("total nodes {0}, total edges {1}").format(Array::make(nodeInitialTotalCount, edgeInitialTotalCount)));
		}

		for (i = 0; i < CHUNKBUILDER_MAX_VERTICES; i++)
			delete[] vertices[i];
		for (i = 0; i < CHUNKBUILDER_MAX_FACES; i++)
			delete[] faces[i];
		delete[] vertices;
		delete[] faces;
 
		meshArrays[Mesh::ARRAY_VERTEX] = vertexArray;
		meshArrays[Mesh::ARRAY_NORMAL] = normalArray;
		//meshArrays[Mesh::ARRAY_TEX_UV] = uvArray;
		meshArrays[Mesh::ARRAY_INDEX] = indexArray;
		
		meshData[0] = meshArrays;
		meshData[1] = collisionArray;
		
		game->call_deferred("build_chunk", meshData, chunk.get(), world.get());
	}
	catch (const std::exception & e) {
		std::cerr << boost::diagnostic_information(e);
	}

	stop = bpt::microsec_clock::local_time();
	dur = stop - start;
	ms = dur.total_milliseconds();

	Godot::print(String("chunk at {0} built in {1} ms").format(Array::make(chunk->getOffset(), ms)));
	chunk->setNavigatable();
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
