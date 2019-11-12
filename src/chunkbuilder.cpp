#include "chunkbuilder.h"
#include <iostream>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <atomic>

namespace bpt = boost::posix_time;
using namespace std;
using namespace godot;

void ChunkBuilder::Worker::run(Chunk* chunk, Node* game) {
	bpt::ptime start, stop;
	bpt::time_duration dur;
	long ms = 0;

	start = bpt::microsec_clock::local_time();

	auto staticBody = StaticBody::_new();
	auto polygonShape = ConcavePolygonShape::_new();
	auto meshInstance = MeshInstance::_new();
	auto mesh = ArrayMesh::_new();

	float* vertices = ChunkBuilder::getVerticesPool().borrow();
	//float* vertices = new float[MAX_VERTICES_SIZE];
	//memset(vertices, 0, MAX_VERTICES_SIZE);

	chunk->buildVolume();

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
	vertexArray.resize(amountVertices);
	normalArray.resize(amountVertices);
	uvArray.resize(amountVertices);
	indexArray.resize(amountIndices);
	collisionArray.resize(amountIndices);

	PoolVector3Array::Write vertexArrayWrite = vertexArray.write();
	PoolVector3Array::Write normalArrayWrite = normalArray.write();
	PoolVector2Array::Write uvArrayWrite = uvArray.write();
	PoolIntArray::Write indexArrayWrite = indexArray.write();
	PoolVector3Array::Write collisionArrayWrite = collisionArray.write();

	for (int i = 0, n = 0; i < offset; i += VERTEX_SIZE, n++) {
		vertexArrayWrite[n] = Vector3(vertices[i], vertices[i + 1], vertices[i + 2]);
		normalArrayWrite[n] = Vector3(vertices[i + 3], vertices[i + 4], vertices[i + 5]);
		uvArrayWrite[n] = Vector2(vertices[i + 6], vertices[i + 7]);
	}

	for (int i = 0, j = 0, n = 0; i < amountIndices; i += 6, j += 4) {
		indexArrayWrite[n++] = j + 2;
		indexArrayWrite[n++] = j + 1;
		indexArrayWrite[n++] = j;
		indexArrayWrite[n++] = j;
		indexArrayWrite[n++] = j + 3;
		indexArrayWrite[n++] = j + 2;
		collisionArrayWrite[i] = vertexArrayWrite[j + 2];
		collisionArrayWrite[i + 1] = vertexArrayWrite[j + 1];
		collisionArrayWrite[i + 2] = vertexArrayWrite[j];
		collisionArrayWrite[i + 3] = vertexArrayWrite[j];
		collisionArrayWrite[i + 4] = vertexArrayWrite[j + 3];
		collisionArrayWrite[i + 5] = vertexArrayWrite[j + 2];
	}

	arrays[Mesh::ARRAY_VERTEX] = vertexArray;
	arrays[Mesh::ARRAY_NORMAL] = normalArray;
	arrays[Mesh::ARRAY_TEX_UV] = uvArray;
	arrays[Mesh::ARRAY_INDEX] = indexArray;

	mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);
	meshInstance->set_mesh(mesh);

	polygonShape->set_faces(collisionArray);
	int ownerId = staticBody->create_shape_owner(staticBody);
	staticBody->shape_owner_add_shape(ownerId, polygonShape);
	meshInstance->add_child(staticBody);

	Variant v = game->call_deferred("addMeshInstance", meshInstance);
	ChunkBuilder::getVerticesPool().ret(vertices);
	//delete[] vertices;

	stop = bpt::microsec_clock::local_time();
	dur = stop - start;
	ms = dur.total_milliseconds();

	cout << "chunk build in " << ms << " ms" << endl;
}

void ChunkBuilder::build(Chunk* chunk, Node* game) {
	Worker* worker = new Worker();
	ioService.post(boost::bind(&Worker::run, worker, chunk, game));
}

