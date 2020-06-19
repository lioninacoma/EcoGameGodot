#include "quadtreebuilder.h"
#include "ecogame.h"
#include "voxelworld.h"
#include "threadpool.h"

using namespace godot;

QuadTreeBuilder::QuadTreeBuilder(std::shared_ptr<VoxelWorld> world) {
	QuadTreeBuilder::world = world;
	QuadTreeBuilder::threadStarted = false;
}

void QuadTreeBuilder::buildQuadTree(std::shared_ptr<quadsquare> quadtree, Vector3 cameraPosition) {
	if (!quadtree) return;
	Node* parent = world->get_parent();

	bpt::ptime start, stop;
	bpt::time_duration dur;
	long ms = 0;

	start = bpt::microsec_clock::local_time();
	int i, j, n, amountVertices, amountIndices, amountFaces;

	float** vertices = new float* [QUADTREEBUILDER_MAX_VERTICES];
	for (i = 0; i < QUADTREEBUILDER_MAX_VERTICES; i++) {
		vertices[i] = new float[QUADTREEBUILDER_VERTEX_SIZE];
		memset(vertices[i], 0, QUADTREEBUILDER_VERTEX_SIZE * sizeof(float));
	}

	int** faces = new int* [QUADTREEBUILDER_MAX_FACES];
	for (i = 0; i < QUADTREEBUILDER_MAX_FACES; i++) {
		faces[i] = new int[QUADTREEBUILDER_FACE_SIZE];
		memset(faces[i], 0, QUADTREEBUILDER_FACE_SIZE * sizeof(int));
	}

	try {
		BUILD_MESH_MUTEX.lock();
		int* counts = new int[2];
		counts[0] = 0; counts[1] = 0;
		const float viewerLocation[3] = { cameraPosition.x, cameraPosition.y, cameraPosition.z };
		quadcornerdata root = quadcornerdata();
		quadtree->update(root, viewerLocation);
		quadtree->buildVertices(root, vertices, faces, counts);
		BUILD_MESH_MUTEX.unlock();

		if (counts[0] <= 0) {
			for (i = 0; i < QUADTREEBUILDER_MAX_VERTICES; i++)
				delete[] vertices[i];
			for (i = 0; i < QUADTREEBUILDER_MAX_FACES; i++)
				delete[] faces[i];
			delete[] vertices;
			delete[] faces;
			delete[] counts;

			stop = bpt::microsec_clock::local_time();
			dur = stop - start;
			ms = dur.total_milliseconds();

			parent->call_deferred("delete_quadtree", quadtree.get(), world.get());

			Godot::print(String("quadsquare at {0} deleted").format(Array::make(quadtree->getOffset(), ms)));
			quadtree->setBuilding(false);
			BUILD_QUEUE_CV.notify_one();
			return;
		}

		Array meshData;
		Array meshArrays;
		unordered_map<size_t, Vector3> nodePoints;

		amountVertices = counts[0];
		amountFaces = counts[1];
		amountIndices = amountFaces * 3;

		Godot::print(String("amountVertices: {0}, amountFaces: {1}").format(Array::make(amountVertices, amountFaces)));

		PoolVector3Array vertexArray;
		PoolVector3Array normalArray;
		PoolIntArray indexArray;
		PoolVector3Array collisionArray;

		meshData.resize(2);
		meshArrays.resize(Mesh::ARRAY_MAX);
		vertexArray.resize(amountVertices);
		normalArray.resize(amountVertices);
		indexArray.resize(amountIndices);
		collisionArray.resize(amountIndices);

		PoolVector3Array::Write vertexArrayWrite = vertexArray.write();
		PoolVector3Array::Write normalArrayWrite = normalArray.write();
		PoolIntArray::Write indexArrayWrite = indexArray.write();
		PoolVector3Array::Write collisionArrayWrite = collisionArray.write();

		for (i = 0; i < amountVertices; i++) {
			vertexArrayWrite[i] = Vector3(vertices[i][0], vertices[i][1], vertices[i][2]);
			normalArrayWrite[i] = Vector3(0, 0, 0);
		}

		for (i = 0, n = 0; i < amountFaces; i++, n += 3) {
			indexArrayWrite[n] = faces[i][2];
			indexArrayWrite[n + 1] = faces[i][1];
			indexArrayWrite[n + 2] = faces[i][0];

			Vector3 x0 = vertexArray[faces[i][0]];
			Vector3 x1 = vertexArray[faces[i][1]];
			Vector3 x2 = vertexArray[faces[i][2]];
			Vector3 v0 = x0 - x2;
			Vector3 v1 = x1 - x2;
			Vector3 normal = v0.cross(v1).normalized();

			normalArrayWrite[faces[i][0]] = fn::addNormal(normalArrayWrite[faces[i][0]], normal);
			normalArrayWrite[faces[i][1]] = fn::addNormal(normalArrayWrite[faces[i][1]], normal);
			normalArrayWrite[faces[i][2]] = fn::addNormal(normalArrayWrite[faces[i][2]], normal);

			collisionArrayWrite[n] = vertexArray[faces[i][0]];
			collisionArrayWrite[n + 1] = vertexArray[faces[i][1]];
			collisionArrayWrite[n + 2] = vertexArray[faces[i][2]];
		}

		for (i = 0; i < QUADTREEBUILDER_MAX_VERTICES; i++)
			delete[] vertices[i];
		for (i = 0; i < QUADTREEBUILDER_MAX_FACES; i++)
			delete[] faces[i];
		delete[] vertices;
		delete[] faces;
		delete[] counts;

		meshArrays[Mesh::ARRAY_VERTEX] = vertexArray;
		meshArrays[Mesh::ARRAY_NORMAL] = normalArray;
		meshArrays[Mesh::ARRAY_INDEX] = indexArray;

		meshData[0] = meshArrays;
		meshData[1] = collisionArray;

		parent->call_deferred("build_quadtree", meshData, quadtree.get(), world.get());
	}
	catch (const std::exception & e) {
		std::cerr << boost::diagnostic_information(e);
	}

	stop = bpt::microsec_clock::local_time();
	dur = stop - start;
	ms = dur.total_milliseconds();

	Godot::print(String("quadtree at {0} built in {1} ms").format(Array::make(quadtree->getOffset(), ms)));
	quadtree->setBuilding(false);
	BUILD_QUEUE_CV.notify_one();
}

void QuadTreeBuilder::processQueue() {
	while (true) {
		boost::unique_lock<boost::mutex> lock(BUILD_QUEUE_WAIT);
		BUILD_QUEUE_CV.wait(lock);
		if (buildQueue.empty()) continue;

		BUILD_QUEUE_MUTEX.lock();
		queueEntry entry = buildQueue.front();
		buildQueue.pop_front();
		size_t hash = fn::hash(entry.quadtree->getOffset());
		inque.erase(hash);
		BUILD_QUEUE_MUTEX.unlock();

		build(entry.quadtree, entry.cameraPosition);
	}
}

void QuadTreeBuilder::queueQuadTree(std::shared_ptr<quadsquare> quadtree, Vector3 cameraPosition) {
	boost::unique_lock<boost::mutex> lock(BUILD_QUEUE_MUTEX);

	size_t hash = fn::hash(quadtree->getOffset());
	if (inque.find(hash) != inque.end()) return;
	inque.insert(hash);
	queueEntry entry;
	entry.quadtree = quadtree;
	entry.cameraPosition = cameraPosition;
	buildQueue.push_back(entry);
}

void QuadTreeBuilder::build(std::shared_ptr<quadsquare> quadtree, Vector3 cameraPosition) {
	if (!threadStarted) {
		threadStarted = true;
		queueThread = std::make_unique<boost::thread>(&QuadTreeBuilder::processQueue, this);
	}

	if (quadtree->isBuilding()) {
		queueQuadTree(quadtree, cameraPosition);
		return;
	}

	quadtree->setBuilding(true);
	ThreadPool::get()->submitTask(boost::bind(&QuadTreeBuilder::buildQuadTree, this, quadtree, cameraPosition));
}
