#include "chunkbuilder.h"
#include "ecogame.h"
#include "navigator.h"

using namespace godot;

void ChunkBuilder::Worker::run(Chunk* chunk, Node* game) {
	//Godot::print(String("building chunk at {0} ...").format(Array::make(chunk->getOffset())));
	if (!chunk || !game) return;

	bpt::ptime start, stop;
	bpt::time_duration dur;
	long ms = 0;

	start = bpt::microsec_clock::local_time();

	if (!chunk->getMeshInstanceId()) {
		if (!chunk->buildVolume()) return;
	}

	Array meshes;
	float** buffers;

	buffers = Worker::getVerticesPool().borrow(TYPES);

	for (int i = 0; i < TYPES; i++) {
		memset(buffers[i], 0, MAX_VERTICES_SIZE * sizeof(*buffers[i]));
	}

	vector<int> offsets = meshBuilder.buildVertices(chunk, buffers, TYPES);
	Navigator::get()->updateGraph(chunk, game);

	for (int o = 0; o < offsets.size(); o++) {
		int offset = offsets[o];
		if (offset <= 0) continue;

		float* vertices = buffers[o];
		int amountVertices = offset / VERTEX_SIZE;
		int amountIndices = amountVertices / 2 * 3;

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

		for (int i = 0, n = 0; i < offset; i += VERTEX_SIZE, n++) {
			vertexArrayWrite[n] = Vector3(vertices[i], vertices[i + 1], vertices[i + 2]);
			normalArrayWrite[n] = Vector3(vertices[i + 3], vertices[i + 4], vertices[i + 5]);
			uvArrayWrite[n] = Vector2(vertices[i + 6], vertices[i + 7]);
		}

		for (int i = 0, j = 0; i < amountIndices; i += 6, j += 4) {
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

	Variant v = game->call_deferred("build_chunk", meshes, Ref<Chunk>(chunk));

	Worker::getVerticesPool().ret(buffers, TYPES);

	stop = bpt::microsec_clock::local_time();
	dur = stop - start;
	ms = dur.total_milliseconds();

	Godot::print(String("chunk at {0} built in {1} ms").format(Array::make(chunk->getOffset(), ms)));
	chunk->setBuilding(false);
}

void ChunkBuilder::build(Chunk* chunk, Node* game) {
	Worker* worker = new Worker();
	if (chunk->isBuilding()) {
		Godot::print(String("chunk at {0} is already building").format(Array::make(chunk->getOffset())));
		return;
	}
	chunk->setBuilding(true);
	ThreadPool::get()->submitTask(boost::bind(&Worker::run, worker, chunk, game));
}
