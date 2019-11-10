#include "chunkbuilder.h"
#include <iostream>
#include <boost/date_time/posix_time/posix_time_types.hpp>
namespace bpt = boost::posix_time;
using namespace std;
using namespace godot;

//void ChunkBuilder::Worker::run(boost::fibers::promise<int>& p) {
//	int ret = 100;
//	p.set_value(ret);
//}

void ChunkBuilder::Worker::run(Chunk* chunk, Node* game) {
	auto staticBody = Ref<ConcavePolygonShape>(StaticBody::_new());
	auto polygonShape = Ref<ConcavePolygonShape>(ConcavePolygonShape::_new());
	auto meshInstance = MeshInstance::_new();
	auto st = Ref<SurfaceTool>(SurfaceTool::_new());
	auto mesh = Ref<ArrayMesh>(ArrayMesh::_new());
	//auto collFaces = PoolVector3Array();

	float* vertices = new float[BUFFER_SIZE];
	memset(vertices, 0, sizeof(*vertices));

	chunk->buildVolume();

	bpt::ptime start, stop;
	start = bpt::microsec_clock::local_time();

	int offset = meshBuilder->buildVertices(chunk->getOffset(), chunk->getVolume(), vertices);
	
	stop = bpt::microsec_clock::local_time();
	bpt::time_duration dur = stop - start;
	long milliseconds = dur.total_milliseconds();
	cout << milliseconds << endl;
	
	int amountVertices = offset / VERTEX_SIZE;
	int amountIndices = offset / 2 * 3;
	//cout << "amount vertices: " << amountVertices << endl;

	Array arrays;
	arrays.resize(Mesh::ARRAY_MAX);

	PoolVector3Array vertexArray;
	vertexArray.resize(amountVertices);

	PoolVector3Array normalArray;
	normalArray.resize(amountVertices);

	PoolVector2Array uvArray;
	uvArray.resize(amountVertices);

	PoolIntArray indexArray;
	indexArray.resize(amountIndices);

	for (int i = 0, n = 0; i < offset; i += VERTEX_SIZE, n++) {
		vertexArray.set(n, Vector3(vertices[i], vertices[i + 1], vertices[i + 2]));
		normalArray.set(n, Vector3(vertices[i + 3], vertices[i + 4], vertices[i + 5]));
		uvArray.set(n, Vector2(vertices[i + 6], vertices[i + 7]));
	}

	for (int i = 0, j = 0, n = 0; i < amountIndices; i += 6, j += 4) {
		indexArray.set(n++, j + 2);
		indexArray.set(n++, j + 1);
		indexArray.set(n++, j);
		indexArray.set(n++, j);
		indexArray.set(n++, j + 3);
		indexArray.set(n++, j + 2);
	}

	arrays[Mesh::ARRAY_VERTEX] = vertexArray;
	arrays[Mesh::ARRAY_NORMAL] = normalArray;
	arrays[Mesh::ARRAY_TEX_UV] = uvArray;
	arrays[Mesh::ARRAY_INDEX] = indexArray;

	mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);
	meshInstance->set_mesh(mesh);

	game->call_deferred("addMeshInstance", meshInstance);

	//st->begin(Mesh::PRIMITIVE_TRIANGLES);

	//int i = 0;
	//int j = 0;
	//int amountIndices = amountVertices / 2 * 3;
	////collFaces.resize(amountIndices);

	//while (i < amountIndices) {
	//	st->add_index(j + 2);
	//	st->add_index(j + 1);
	//	st->add_index(j);
	//	st->add_index(j);
	//	st->add_index(j + 3);
	//	st->add_index(j + 2);
	//	/*collFaces[i] = vertices[j + 2][0];
	//	collFaces[i + 1] = vertices[j + 1][0];
	//	collFaces[i + 2] = vertices[j][0];
	//	collFaces[i + 3] = vertices[j][0];
	//	collFaces[i + 4] = vertices[j + 3][0];
	//	collFaces[i + 5] = vertices[j + 2][0];*/
	//	i += 6;
	//	j += 4;
	//}

	////st->set_material(terrainMaterial);
	//for v in vertices {
	//	st.add_uv(v[1])
	//	st.add_vertex(v[0])

	//	st.generate_normals()
	//	st.commit(mesh)
	//	meshInstance.mesh = mesh

	//	polygonShape.set_faces(collFaces)
	//	var ownerId = staticBody.create_shape_owner(staticBody)
	//	staticBody.shape_owner_add_shape(ownerId, polygonShape)
	//	meshInstance.add_child(staticBody)
	//}
}

void ChunkBuilder::build(Chunk* chunk, Node* game) {
	Worker* worker = new Worker();
	ioService.post(boost::bind(&Worker::run, worker, chunk, game));
}

