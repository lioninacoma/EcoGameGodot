#include "chunkbuilder.h"
#include "ecogame.h"

using namespace godot;

void ChunkBuilder::Worker::run(Chunk* chunk, Node* game) {
	if (!chunk) return;

	bpt::ptime start, stop;
	bpt::time_duration dur;
	long ms = 0;

	start = bpt::microsec_clock::local_time();

	if (!chunk->getMeshInstanceId()) {
		if (!chunk->buildVolume()) return;
	}

	Array meshes;
	vector<float*> buffers;
	const int types = 3;

	for (int i = 0; i < types; i++) {
		float* vertices = Worker::getVerticesPool().borrow();
		memset(vertices, 0, MAX_VERTICES_SIZE * sizeof(*vertices));
		buffers.push_back(vertices);
	}

	vector<int> offsets = meshBuilder.buildVertices(chunk, buffers);

	int type = 1;
	for (int i = 0; i < offsets.size(); i++) {
		int offset = offsets[i];
		if (offset <= 0) continue;

		float* vertices = buffers[type - 1];
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
		meshData.push_back(type);
		meshData.push_back(offset);

		meshes.push_back(meshData);
		
		type++;
	}

	//Variant v = game->call("build_mesh_instance", arrays, collisionArray);
	Variant v = game->call_deferred("build_mesh_instance", meshes, Ref<Chunk>(chunk));

	for (int i = 0; i < types; i++) {
		float* vertices = buffers[i];
		Worker::getVerticesPool().ret(vertices);
	}
	

	stop = bpt::microsec_clock::local_time();
	dur = stop - start;
	ms = dur.total_milliseconds();

	cout << "chunk build in " << ms << " ms" << endl;
}

void ChunkBuilder::build(Chunk* chunk, Node* game) {
	Worker* worker = new Worker();
	ioService.post(boost::bind(&Worker::run, worker, chunk, game));
}
