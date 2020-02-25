#include "chunkbuilder.h"
#include "ecogame.h"
#include "threadpool.h"

#include <boost/exception/diagnostic_information.hpp> 
#include <boost/exception_ptr.hpp> 

using namespace godot;

ChunkBuilder::ChunkBuilder() {
	ChunkBuilder::threadStarted = false;
}

void ChunkBuilder::buildChunk(std::shared_ptr<Chunk> chunk, Node* game) {
	//Godot::print(String("building chunk at {0} ...").format(Array::make(chunk->getOffset())));
	if (!chunk || !game) return;

	bpt::ptime start, stop;
	bpt::time_duration dur;
	long ms = 0;

	start = bpt::microsec_clock::local_time();

	if (!chunk->getMeshInstanceId()) {
		if (!chunk->buildVolume()) return;
	}

	int i, j, o, n, offset, amountVertices, amountIndices;
	Array meshes;
	float** buffers;
	float* vertices;

	buffers = getVerticesPool().borrow(TYPES);

	try {
		for (i = 0; i < TYPES; i++) {
			memset(buffers[i], 0, MAX_VERTICES_SIZE * sizeof(*buffers[i]));
		}

		BUILD_MESH_MUTEX.lock();
		vector<int> offsets = meshBuilder.buildVertices(chunk, buffers, TYPES, game);
		BUILD_MESH_MUTEX.unlock();

		for (o = 0; o < offsets.size(); o++) {
			offset = offsets[o];
			if (offset <= 0) continue;

			vertices = buffers[o];
			amountVertices = offset / VERTEX_SIZE;
			amountIndices = amountVertices / 2 * 3;

			Array meshData;
			Array meshArrays;
			PoolVector3Array vertexArray;
			PoolVector3Array normalArray;
			PoolVector2Array uvArray;
			PoolIntArray indexArray;
			PoolVector3Array collisionArray;

			meshArrays.resize(Mesh::ARRAY_MAX);
			vertexArray.resize(amountVertices);
			normalArray.resize(amountVertices);
			uvArray.resize(amountVertices);
			indexArray.resize(amountIndices);
			collisionArray.resize(amountIndices);

			PoolVector3Array::Write vertexArrayWrite = vertexArray.write();
			PoolVector3Array::Write normalArrayWrite = normalArray.write();
			PoolVector3Array::Write collisionArrayWrite = collisionArray.write();
			PoolVector2Array::Write uvArrayWrite = uvArray.write();
			PoolIntArray::Write indexArrayWrite = indexArray.write();

			for (i = 0, n = 0; i < offset; i += VERTEX_SIZE, n++) {
				vertexArrayWrite[n] = Vector3(vertices[i], vertices[i + 1], vertices[i + 2]);
				normalArrayWrite[n] = Vector3(vertices[i + 3], vertices[i + 4], vertices[i + 5]);
				uvArrayWrite[n] = Vector2(vertices[i + 6], vertices[i + 7]);
			}

			for (i = 0, j = 0; i < amountIndices; i += 6, j += 4) {
				indexArrayWrite[i + 0] = j + 2;
				indexArrayWrite[i + 1] = j + 1;
				indexArrayWrite[i + 2] = j;
				indexArrayWrite[i + 3] = j;
				indexArrayWrite[i + 4] = j + 3;
				indexArrayWrite[i + 5] = j + 2;
				collisionArrayWrite[i + 0] = vertexArrayWrite[j + 2];
				collisionArrayWrite[i + 1] = vertexArrayWrite[j + 1];
				collisionArrayWrite[i + 2] = vertexArrayWrite[j];
				collisionArrayWrite[i + 3] = vertexArrayWrite[j];
				collisionArrayWrite[i + 4] = vertexArrayWrite[j + 3];
				collisionArrayWrite[i + 5] = vertexArrayWrite[j + 2];
			}

			meshArrays[Mesh::ARRAY_VERTEX] = vertexArray;
			meshArrays[Mesh::ARRAY_NORMAL] = normalArray;
			meshArrays[Mesh::ARRAY_TEX_UV] = uvArray;
			meshArrays[Mesh::ARRAY_INDEX] = indexArray;

			meshData.push_back(meshArrays);
			meshData.push_back(collisionArray);
			meshData.push_back(o + 1);
			meshData.push_back(offset);

			meshes.push_back(meshData);
		}

		game->call_deferred("build_chunk", meshes, chunk.get());
	}
	catch (const std::exception & e) {
		std::cerr << boost::diagnostic_information(e);
	}

	getVerticesPool().ret(buffers, TYPES);

	stop = bpt::microsec_clock::local_time();
	dur = stop - start;
	ms = dur.total_milliseconds();

	Godot::print(String("chunk at {0} built in {1} ms").format(Array::make(chunk->getOffset(), ms)));
	chunk->setBuilding(false);
	BUILD_QUEUE_CV.notify_one();
}

void ChunkBuilder::processQueue(Node* game) {
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

		build(chunk, game);
	}
}

void ChunkBuilder::queueChunk(std::shared_ptr<Chunk> chunk) {
	boost::unique_lock<boost::mutex> lock(BUILD_QUEUE_MUTEX);

	size_t hash = fn::hash(chunk->getOffset());
	if (inque.find(hash) != inque.end()) return;
	inque.insert(hash);
	buildQueue.push_back(chunk);
}

void ChunkBuilder::build(std::shared_ptr<Chunk> chunk, Node* game) {
	if (!threadStarted) {
		threadStarted = true;
		queueThread = std::make_shared<boost::thread>(&ChunkBuilder::processQueue, this, game);
	}

	if (chunk->isBuilding()) {
		//Godot::print(String("chunk at {0} is already building").format(Array::make(chunk->getOffset())));
		queueChunk(chunk);
		return;
	}

	chunk->setBuilding(true);
	ThreadPool::get()->submitTask(boost::bind(&ChunkBuilder::buildChunk, this, chunk, game));
}
