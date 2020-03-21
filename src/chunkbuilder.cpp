#include "chunkbuilder.h"
#include "ecogame.h"
#include "threadpool.h"
#include "navigator.h"

#include <boost/exception/diagnostic_information.hpp> 
#include <boost/exception_ptr.hpp> 

using namespace godot;

ChunkBuilder::ChunkBuilder() {
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

	int amountVertices, amountIndices, amountFaces;
	const int VERTEX_BUFFER_SIZE = CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z * 6 * 4;

	float** vertices = new float*[VERTEX_BUFFER_SIZE];
	for (int i = 0; i < VERTEX_BUFFER_SIZE; i++) {
		vertices[i] = new float[3];
		memset(vertices[i], 0, 3 * sizeof(vertices[i][0]));
	}

	int** faces = new int* [VERTEX_BUFFER_SIZE / 3];
	for (int i = 0; i < VERTEX_BUFFER_SIZE / 3; i++) {
		faces[i] = new int[3];
		memset(faces[i], 0, 3 * sizeof(faces[i][0]));
	}

	if (!chunk->getMeshInstanceId()) {
		chunk->buildVolume();
	}
	
	try {
		BUILD_MESH_MUTEX.lock();
		Array data = meshBuilder.buildVertices(chunk, vertices, faces);
		BUILD_MESH_MUTEX.unlock();

		if (data.empty() || !bool(data[0])) {
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

		amountVertices = data[0];
		amountFaces = data[1];
		amountIndices = amountFaces * 3;
		
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

		for (int i = 0; i < amountVertices; i++) {
			float* v = vertices[i];
			//Godot::print(String("v {0}").format(Array::make(Vector3(v[0], v[1], v[2]))));
			vertexArrayWrite[i] = Vector3(v[0] - 0.5, v[1] - 0.5, v[2] - 0.5);
			normalArrayWrite[i] = Vector3(0, 0, 0);
		}

		for (int i = 0, n = 0; i < amountFaces; i++, n += 3) {
			int* f = faces[i];
			indexArrayWrite[n] = f[2];
			indexArrayWrite[n + 1] = f[1];
			indexArrayWrite[n + 2] = f[0];

			Vector3 x0 = vertexArray[f[2]];
			Vector3 x1 = vertexArray[f[1]];
			Vector3 x2 = vertexArray[f[0]];
			Vector3 v0 = x0 - x2;
			Vector3 v1 = x1 - x2;
			Vector3 normal = v1.cross(v0).normalized();

			normalArrayWrite[f[0]] = addNormal(normalArrayWrite[f[0]], normal);
			normalArrayWrite[f[1]] = addNormal(normalArrayWrite[f[1]], normal);
			normalArrayWrite[f[2]] = addNormal(normalArrayWrite[f[2]], normal);

			collisionArrayWrite[n] = vertexArray[f[2]];
			collisionArrayWrite[n + 1] = vertexArray[f[1]];
			collisionArrayWrite[n + 2] = vertexArray[f[0]];

			Navigator::get()->addFaceNodes(x0, x1, x2, chunk.get());
		}
 
		meshArrays[Mesh::ARRAY_VERTEX] = vertexArray;
		meshArrays[Mesh::ARRAY_NORMAL] = normalArray;
		//meshArrays[Mesh::ARRAY_TEX_UV] = uvArray;
		meshArrays[Mesh::ARRAY_INDEX] = indexArray;
		
		meshData[0] = meshArrays;
		meshData[1] = collisionArray;
		
		game->call_deferred("build_chunk_smooth", meshData, chunk.get());
	}
	catch (const std::exception & e) {
		std::cerr << boost::diagnostic_information(e);
	}

	delete[] vertices;
	delete[] faces;

	stop = bpt::microsec_clock::local_time();
	dur = stop - start;
	ms = dur.total_milliseconds();

	Godot::print(String("chunk at {0} built in {1} ms").format(Array::make(chunk->getOffset(), ms)));
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
