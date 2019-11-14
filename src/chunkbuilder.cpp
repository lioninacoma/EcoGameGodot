#include "chunkbuilder.h"
#include <iostream>
#include <boost/date_time/posix_time/posix_time_types.hpp>

namespace bpt = boost::posix_time;
using namespace std;
using namespace godot;

void ChunkBuilder::Worker::run(Chunk* chunk, Node* game) {
	if (!chunk || !game) return;

	bpt::ptime start, stop;
	bpt::time_duration dur;
	long ms = 0;

	start = bpt::microsec_clock::local_time();

	if (!chunk->buildVolume()) return;

	float* vertices = Worker::getVerticesPool().borrow();
	memset(vertices, 0, MAX_VERTICES_SIZE);

	int offset = meshBuilder.buildVertices(chunk, vertices);
	int amountVertices = offset / VERTEX_SIZE;
	int amountIndices = amountVertices / 2 * 3;

	Array arrays;
	PoolVector3Array vertexArray;
	PoolVector3Array normalArray;
	PoolVector2Array uvArray;
	PoolIntArray indexArray;
	PoolVector3Array collisionArray;

	arrays.resize(Mesh::ARRAY_MAX);
	
	for (int i = 0; i < offset; i += VERTEX_SIZE) {
		vertexArray.push_back(Vector3(vertices[i], vertices[i + 1], vertices[i + 2]));
		normalArray.push_back(Vector3(vertices[i + 3], vertices[i + 4], vertices[i + 5]));
		uvArray.push_back(Vector2(vertices[i + 6], vertices[i + 7]));
	}

	for (int i = 0, j = 0; i < amountIndices; i += 6, j += 4) {
		indexArray.push_back(j + 2);
		indexArray.push_back(j + 1);
		indexArray.push_back(j);
		indexArray.push_back(j);
		indexArray.push_back(j + 3);
		indexArray.push_back(j + 2);
		collisionArray.push_back(vertexArray[j + 2]);
		collisionArray.push_back(vertexArray[j + 1]);
		collisionArray.push_back(vertexArray[j]);
		collisionArray.push_back(vertexArray[j]);
		collisionArray.push_back(vertexArray[j + 3]);
		collisionArray.push_back(vertexArray[j + 2]);
	}

	arrays[Mesh::ARRAY_VERTEX] = vertexArray;
	arrays[Mesh::ARRAY_NORMAL] = normalArray;
	arrays[Mesh::ARRAY_TEX_UV] = uvArray;
	arrays[Mesh::ARRAY_INDEX] = indexArray;

	//Variant v = game->call("build_mesh_instance", arrays, collisionArray);
	Variant v = game->call_deferred("build_mesh_instance", arrays, collisionArray);

	Worker::getVerticesPool().ret(vertices);

	stop = bpt::microsec_clock::local_time();
	dur = stop - start;
	ms = dur.total_milliseconds();

	cout << "chunk build in " << ms << " ms" << endl;
}

void ChunkBuilder::build(Chunk* chunk, Node* game) {
	Worker* worker = new Worker();
	ioService.post(boost::bind(&Worker::run, worker, chunk, game));
}
