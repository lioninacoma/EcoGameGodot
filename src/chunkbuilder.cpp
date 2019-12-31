#include "chunkbuilder.h"
#include "ecogame.h"

using namespace godot;

void ChunkBuilder::Worker::run(Chunk* chunk, Node* game) {
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
	const int types = 6;

	buffers = Worker::getVerticesPool().borrow(types);

	for (int i = 0; i < types; i++) {
		memset(buffers[i], 0, MAX_VERTICES_SIZE * sizeof(*buffers[i]));
	}

	vector<int> offsets = meshBuilder.buildVertices(chunk, buffers, types);
	cout << chunk->getAmountNodes() << endl;

	auto points = *chunk->getNodes();
	int y0, y1, y2, areaSize;
	int drawOffsetY = 1;
	int maxDeltaY = 0;
	//double maxDst = 5.0;
	double m, o;

	auto dots = ImmediateGeometry::_new();
	dots->begin(Mesh::PRIMITIVE_POINTS);
	dots->set_color(Color(1, 0, 0, 1));

	std::list<Point> L;
	for (Vector3 n : points) {
		dots->add_vertex(n + Vector3(0, drawOffsetY, 0));
		L.push_front(Point(n.x, n.y, n.z));
	}

	Triangulation T(L.begin(), L.end());

	dots->end();
	game->call_deferred("draw_debug_dots", dots);

	auto geo = ImmediateGeometry::_new();
	geo->begin(Mesh::PRIMITIVE_LINES);
	geo->set_color(Color(0, 1, 0, 1));

	//for (const auto& vertex : vd.vertices()) {
	//	std::vector<int> triangle;
	//	auto edge = vertex.incident_edge();
	//	do {
	//		auto cell = edge->cell();
	//		assert(cell->contains_point());

	//		triangle.push_back(cell->source_index());
	//		if (triangle.size() == 3) {
	//			geo->add_vertex(Vector3(points[triangle[0]].x, points[triangle[0]].y + drawOffsetY, points[triangle[0]].z));
	//			geo->add_vertex(Vector3(points[triangle[1]].x, points[triangle[1]].y + drawOffsetY, points[triangle[1]].z));
	//			geo->add_vertex(Vector3(points[triangle[1]].x, points[triangle[1]].y + drawOffsetY, points[triangle[1]].z));
	//			geo->add_vertex(Vector3(points[triangle[2]].x, points[triangle[2]].y + drawOffsetY, points[triangle[2]].z));
	//			geo->add_vertex(Vector3(points[triangle[2]].x, points[triangle[2]].y + drawOffsetY, points[triangle[2]].z));
	//			geo->add_vertex(Vector3(points[triangle[0]].x, points[triangle[0]].y + drawOffsetY, points[triangle[0]].z));

	//			triangle.erase(triangle.begin() + 1);
	//		}

	//		edge = edge->rot_next();
	//	} while (edge != vertex.incident_edge());
	//}

	geo->end();
	game->call_deferred("draw_debug", geo);

	for (int i = 0; i < offsets.size(); i++) {
		int offset = offsets[i];
		if (offset <= 0) continue;

		float* vertices = buffers[i];
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
		meshData.push_back(i + 1);
		meshData.push_back(offset);

		meshes.push_back(meshData);
	}

	Variant v = game->call_deferred("build_mesh_instance", meshes, Ref<Chunk>(chunk));

	Worker::getVerticesPool().ret(buffers, types);

	stop = bpt::microsec_clock::local_time();
	dur = stop - start;
	ms = dur.total_milliseconds();

	cout << "chunk build in " << ms << " ms" << endl;
}

void ChunkBuilder::build(Chunk* chunk, Node* game) {
	Worker* worker = new Worker();

	
	ThreadPool::get()->submitTask(boost::bind(&Worker::run, worker, chunk, game));
}
