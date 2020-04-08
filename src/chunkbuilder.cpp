#include "chunkbuilder.h"
#include "ecogame.h"
#include "voxelworld.h"
#include "threadpool.h"
#include "navigator.h"

#include <boost/exception/diagnostic_information.hpp> 
#include <boost/exception_ptr.hpp> 

using namespace godot;

ChunkBuilder::ChunkBuilder(std::shared_ptr<VoxelWorld> world) {
	ChunkBuilder::world = world;
	ChunkBuilder::meshBuilder = std::make_shared<MeshBuilder>(world);
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

		for (i = 0; i < CHUNKBUILDER_MAX_VERTICES; i++)
			delete[] vertices[i];
		for (i = 0; i < CHUNKBUILDER_MAX_FACES; i++)
			delete[] faces[i];
		delete[] vertices;
		delete[] faces;
		
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

		for (i = 0; i < amountVertices; i++) {
			//Godot::print(String("v {0}").format(Array::make(Vector3(v[0], v[1], v[2]))));
			vertexArrayWrite[i] = Vector3(chunkVertices[i][0], chunkVertices[i][1], chunkVertices[i][2]);
			normalArrayWrite[i] = Vector3(0, 0, 0);
		}

		for (i = 0, n = 0; i < amountFaces; i++, n += 3) {
			indexArrayWrite[n] = chunkFaces[i][2];
			indexArrayWrite[n + 1] = chunkFaces[i][1];
			indexArrayWrite[n + 2] = chunkFaces[i][0];

			Vector3 x0 = vertexArray[chunkFaces[i][2]];
			Vector3 x1 = vertexArray[chunkFaces[i][1]];
			Vector3 x2 = vertexArray[chunkFaces[i][0]];
			Vector3 v0 = x0 - x2;
			Vector3 v1 = x1 - x2;
			Vector3 normal = v1.cross(v0).normalized();

			normalArrayWrite[chunkFaces[i][0]] = addNormal(normalArrayWrite[chunkFaces[i][0]], normal);
			normalArrayWrite[chunkFaces[i][1]] = addNormal(normalArrayWrite[chunkFaces[i][1]], normal);
			normalArrayWrite[chunkFaces[i][2]] = addNormal(normalArrayWrite[chunkFaces[i][2]], normal);

			collisionArrayWrite[n] = vertexArray[chunkFaces[i][2]];
			collisionArrayWrite[n + 1] = vertexArray[chunkFaces[i][1]];
			collisionArrayWrite[n + 2] = vertexArray[chunkFaces[i][0]];
			
			chunk->addFaceNodes(x0, x1, x2);
		}
 
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
		queueThread = std::make_shared<boost::thread>(&ChunkBuilder::processQueue, this);
	}

	if (chunk->isBuilding()) {
		queueChunk(chunk);
		return;
	}

	chunk->setBuilding(true);
	ThreadPool::get()->submitTask(boost::bind(&ChunkBuilder::buildChunk, this, chunk));
}
