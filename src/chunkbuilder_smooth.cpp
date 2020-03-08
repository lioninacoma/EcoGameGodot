#include "chunkbuilder_smooth.h"
#include "ecogame.h"
#include "threadpool.h"

#include <boost/exception/diagnostic_information.hpp> 
#include <boost/exception_ptr.hpp> 

using namespace godot;

ChunkBuilder_Smooth::ChunkBuilder_Smooth() {
	ChunkBuilder_Smooth::threadStarted = false;
}

void ChunkBuilder_Smooth::buildChunk(std::shared_ptr<Chunk> chunk) {
	if (!chunk) return;
	Node* game = EcoGame::get()->getNode();

	bpt::ptime start, stop;
	bpt::time_duration dur;
	long ms = 0;

	start = bpt::microsec_clock::local_time();

	int amountVertices, amountIndices, amountFaces;

	if (!chunk->getMeshInstanceId()) {
		if (!chunk->buildVolume()) return;
	}
	
	try {
		BUILD_MESH_MUTEX.lock();
		Array data = meshBuilder.buildVertices(chunk);
		data = meshBuilder.buildVertices(chunk);
		BUILD_MESH_MUTEX.unlock();

		Array meshData;
		Array meshArrays;
		Array vertices = data[0];
		Array faces = data[1];

		amountVertices = data[2];
		amountFaces = data[3];
		amountIndices = amountFaces * 6;
		
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
			PoolRealArray v = vertices[i];
			vertexArrayWrite[i] = Vector3(v[0], v[1], v[2]);
		}

		for (int i = 0, n = 0; i < amountFaces; i++, n += 6) {
			PoolIntArray f = faces[i];
			indexArrayWrite[n + 0] = f[2];
			indexArrayWrite[n + 1] = f[1];
			indexArrayWrite[n + 2] = f[0];
			indexArrayWrite[n + 3] = f[0];
			indexArrayWrite[n + 4] = f[3];
			indexArrayWrite[n + 5] = f[2];

			Vector3 x0 = vertexArray[f[2]];
			Vector3 x1 = vertexArray[f[1]];
			Vector3 x2 = vertexArray[f[0]];
			Vector3 v0 = x0 - x2;
			Vector3 v1 = x1 - x2;
			Vector3 normal = v1.cross(v0).normalized();

			normalArrayWrite[f[0]] = normal;
			normalArrayWrite[f[1]] = normal;
			normalArrayWrite[f[2]] = normal;
			normalArrayWrite[f[3]] = normal;

			collisionArrayWrite[n + 0] = vertexArray[f[2]];
			collisionArrayWrite[n + 1] = vertexArray[f[1]];
			collisionArrayWrite[n + 2] = vertexArray[f[0]];
			collisionArrayWrite[n + 3] = vertexArray[f[0]];
			collisionArrayWrite[n + 4] = vertexArray[f[3]];
			collisionArrayWrite[n + 5] = vertexArray[f[2]];
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

	stop = bpt::microsec_clock::local_time();
	dur = stop - start;
	ms = dur.total_milliseconds();

	Godot::print(String("chunk at {0} built in {1} ms").format(Array::make(chunk->getOffset(), ms)));
	chunk->setBuilding(false);
	BUILD_QUEUE_CV.notify_one();
}

void ChunkBuilder_Smooth::processQueue() {
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

void ChunkBuilder_Smooth::queueChunk(std::shared_ptr<Chunk> chunk) {
	boost::unique_lock<boost::mutex> lock(BUILD_QUEUE_MUTEX);

	size_t hash = fn::hash(chunk->getOffset());
	if (inque.find(hash) != inque.end()) return;
	inque.insert(hash);
	buildQueue.push_back(chunk);
}

void ChunkBuilder_Smooth::build(std::shared_ptr<Chunk> chunk) {
	if (!threadStarted) {
		threadStarted = true;
		queueThread = std::make_shared<boost::thread>(&ChunkBuilder_Smooth::processQueue, this);
	}

	if (chunk->isBuilding()) {
		queueChunk(chunk);
		return;
	}

	chunk->setBuilding(true);
	ThreadPool::get()->submitTask(boost::bind(&ChunkBuilder_Smooth::buildChunk, this, chunk));
}
