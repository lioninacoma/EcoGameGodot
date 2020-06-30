#include "voxelworld.h"
#include "navigator.h"
#include "threadpool.h"
#include "graphnode.h"
#include "octree.h"

#include <boost/exception/diagnostic_information.hpp>
#include <boost/exception_ptr.hpp>

using namespace godot;

void VoxelWorld::_register_methods() {
	register_method("setSize", &VoxelWorld::setSize);
	register_method("getSize", &VoxelWorld::getSize);
	register_method("setVoxel", &VoxelWorld::setVoxel);
	register_method("getVoxel", &VoxelWorld::getVoxel);
	register_method("buildChunks", &VoxelWorld::buildChunks);
	register_method("buildQuadTrees", &VoxelWorld::buildQuadTrees);
	register_method("navigate", &VoxelWorld::navigate);
	register_method("setIsVoxelFn", &VoxelWorld::setIsVoxelFn);
	register_method("setIsWalkableFn", &VoxelWorld::setIsWalkableFn);
	register_method("_notification", &VoxelWorld::_notification);
}

VoxelWorld::VoxelWorld() {
	VoxelWorld::size = 16;

	chunks = vector<std::shared_ptr<Chunk>>(size * size * size);
	self = std::shared_ptr<VoxelWorld>(this);
	chunkBuilder = std::make_unique<ChunkBuilder>(self);
	quadtreeBuilder = std::make_unique<QuadTreeBuilder>(self);
	navigator = std::make_unique<Navigator>(self);
	quadtrees = vector<quadsquare*>(size_t(1));
	quadtrees[0] = quadsquare::_new();
}

VoxelWorld::~VoxelWorld() {
	chunkBuilder.reset();
}

void VoxelWorld::_init() {

}

std::shared_ptr<Chunk> VoxelWorld::intersection(int x, int y, int z) {
	std::shared_ptr<Chunk> chunk = getChunk(x, y, z);
	if (!chunk) return NULL;

	return chunk;
}

vector<std::shared_ptr<Chunk>> VoxelWorld::getChunksRay(Vector3 from, Vector3 to) {
	from = fn::toChunkCoords(from);
	to = fn::toChunkCoords(to);

	vector<std::shared_ptr<Chunk>> list;
	boost::function<std::shared_ptr<Chunk>(int, int, int)> intersection(boost::bind(&VoxelWorld::intersection, this, _1, _2, _3));
	return Intersection::get<std::shared_ptr<Chunk>>(from, to, false, intersection, list);
}

std::shared_ptr<Chunk> VoxelWorld::getChunk(int x, int y, int z) {
	if (x < 0 || x >= size || y < 0 || y >= size || z < 0 || z >= size) return NULL;
	std::shared_lock<std::shared_mutex> lock(CHUNKS_MUTEX);
	//return chunks[fn::hash(Vector3(x, y, z))];
	return chunks[fn::fi3(x, y, z, size, size)];
}

std::shared_ptr<Chunk> VoxelWorld::getChunk(int i) {
	if (i < 0 || i >= size * size * size) return NULL;
	std::shared_lock<std::shared_mutex> lock(CHUNKS_MUTEX);
	return chunks.at(i);
}

std::shared_ptr<Chunk> VoxelWorld::getChunk(Vector3 position) {
	//Godot::print(String("get chunk at: {0}").format(Array::make(position)));
	if (position.x < 0 || position.x >= size * CHUNK_SIZE
		|| position.y < 0 || position.y >= size * CHUNK_SIZE
		|| position.z < 0 || position.z >= size * CHUNK_SIZE)
		return NULL;

	int cx, cy, cz;
	Vector3 chunkOffset = position;
	chunkOffset = fn::toChunkCoords(chunkOffset);
	cx = (int)chunkOffset.x;
	cy = (int)chunkOffset.y;
	cz = (int)chunkOffset.z;
	auto chunk = getChunk(cx, cy, cz);

	if (!chunk) return NULL;
	return chunk;
}

int VoxelWorld::getSize() {
	return size;
}

void VoxelWorld::_notification(const int64_t what) {
	if (what == Node::NOTIFICATION_READY) {}
}

void VoxelWorld::setSize(float size) {
	VoxelWorld::size = size;
}

void VoxelWorld::navigate(Vector3 startV, Vector3 goalV, int actorInstanceId) {
	/*startV = this->to_local(startV);
	goalV = this->to_local(goalV);*/
	ThreadPool::getNav()->submitTask(boost::bind(&VoxelWorld::navigateTask, this, startV, goalV, actorInstanceId));
}

void VoxelWorld::navigateTask(Vector3 startV, Vector3 goalV, int actorInstanceId) {
	navigator->navigate(startV, goalV, actorInstanceId);
}

void VoxelWorld::setIsVoxelFn(Variant fnRef) {
	Ref<FuncRef> ref = Object::cast_to<FuncRef>(fnRef);
	VoxelWorld::isVoxelFn = ref;
}

void VoxelWorld::setIsWalkableFn(Variant fnRef) {
	Ref<FuncRef> ref = Object::cast_to<FuncRef>(fnRef);
	navigator->setIsWalkableFn(ref);
}

void VoxelWorld::setVoxel(Vector3 position, float radius, bool set) {
	try {
		int x, y, z;
		float s, m = (set) ? -1 : 1;
		Vector3 currentPosition, neighbourPosition;
		std::shared_ptr<Chunk> chunk, neighbour;
		unordered_map<size_t, std::shared_ptr<Chunk>> updatingChunks;
		position += Vector3(0.5, 0.5, 0.5);

		for (z = -radius + position.z; z < radius + position.z; z++)
			for (y = -radius + position.y; y < radius + position.y; y++)
				for (x = -radius + position.x; x < radius + position.x; x++) {
					currentPosition = Vector3(x, y, z);
					if (currentPosition.distance_to(position) > radius) continue;

					chunk = getChunk(currentPosition);
					if (!chunk) continue;

					const Vector3 baseChunkMin = chunk->getOffset();
					updatingChunks.emplace(fn::hash(baseChunkMin), chunk);

					for (int dz = -1; dz < 1; dz++)
						for (int dy = -1; dy < 1; dy++)
							for (int dx = -1; dx < 1; dx++) {
								if (!dx && !dy && !dz) continue;
								const Vector3 offset = Vector3(dx, dy, dz) + currentPosition;
								if (int(offset.x) % CHUNK_SIZE == 0 || int(offset.y) % CHUNK_SIZE == 0 || int(offset.z) % CHUNK_SIZE == 0) {
									neighbour = getChunk(offset);
									if (neighbour && neighbour != chunk)
										updatingChunks.emplace(fn::hash(neighbour->getOffset()), neighbour);
								}
							}

					s = 1.0 - (currentPosition.distance_to(position) / radius);
					s /= 20.0;
					chunk->setVoxel(
						x % CHUNK_SIZE,
						y % CHUNK_SIZE,
						z % CHUNK_SIZE, s * m);
				}

		// First build octree then update mesh. Otherwise there would be cracks.
		for (auto chunk : updatingChunks) {
			const Vector3 baseChunkMin = chunk.second->getOffset();
			try {
				chunk.second->deleteRoot();
				auto root = BuildOctree(baseChunkMin, CHUNK_SIZE, OCTREE_LOD);
				chunk.second->setRoot(root);
			}
			catch (const std::exception & e) {
				//Godot::print(String("chunk at {0} is empty").format(Array::make(baseChunkMin)));
			}
		}

		for (auto chunk : updatingChunks) {
			chunkBuilder->build(chunk.second);
		}
	}
	catch (const std::exception & e) {
		std::cerr << boost::diagnostic_information(e);
	}
}

int VoxelWorld::getVoxel(Vector3 position) {
	int x, y, z;
	auto chunk = getChunk(position);

	if (!chunk) return 0;

	x = position.x;
	y = position.y;
	z = position.z;

	return chunk->getVoxel(
		x % CHUNK_SIZE,
		y % CHUNK_SIZE,
		z % CHUNK_SIZE);
}

std::shared_ptr<GraphNode> VoxelWorld::findClosestNode(Vector3 position) {
	auto chunk = getChunk(position);
	if (!chunk) return NULL;

	std::shared_ptr<GraphNode> closest;
	closest = chunk->getNode(fn::hash(position));
	if (closest) return closest;

	float minDist = numeric_limits<float>::max();
	float dist;
	std::function<void(pair<size_t, std::shared_ptr<GraphNode>>)> lambda = [&](auto next) {
		dist = next.second->getPoint()->distance_to(position);
		if (dist < minDist) {
			minDist = dist;
			closest = next.second;
		}
	};
	chunk->forEachNode(lambda);
	return closest;
}

std::shared_ptr<GraphNode> VoxelWorld::getNode(Vector3 position) {
	auto chunk = getChunk(position);
	if (!chunk) return NULL;
	return chunk->getNode(fn::hash(position));
}

void VoxelWorld::setChunk(int x, int y, int z, std::shared_ptr<Chunk> chunk) {
	if (x < 0 || x >= size || y < 0 || y >= size || z < 0 || z >= size) return;
	std::unique_lock<std::shared_mutex> lock(CHUNKS_MUTEX);
	//chunks[fn::hash(Vector3(x, y, z))] = chunk;
	chunks[fn::fi3(x, y, z, size, size)] = chunk;
}

void VoxelWorld::buildChunksTask() {
	int x, y, z, i;
	std::shared_ptr<Chunk> chunk;
	auto threadpool = new ThreadPool(8);
	cout << "preparing chunks ..." << endl;

	bpt::ptime start, stop;
	bpt::time_duration dur;
	long ms = 0;

	start = bpt::microsec_clock::local_time();

	for (z = 0; z < size; z++) {
		for (y = 0; y < size; y++) {
			for (x = 0; x < size; x++) {
				const Vector3 baseChunkMin = Vector3(x * CHUNK_SIZE, y * CHUNK_SIZE, z * CHUNK_SIZE);
				chunk = std::shared_ptr<Chunk>(Chunk::_new());
				chunk->setOffset(baseChunkMin);
				chunk->setWorld(self);
				setChunk(x, y, z, chunk);
				threadpool->submitTask(boost::bind(&VoxelWorld::prepareChunkTask, this, chunk, OCTREE_LOD));
			}
		}
	}

	threadpool->waitUntilFinished();

	stop = bpt::microsec_clock::local_time();
	dur = stop - start;
	ms = dur.total_milliseconds();

	Godot::print(String("chunks prepared in {0} ms").format(Array::make(ms)));

	for (i = 0; i < size * size * size; i++) {
		chunk = getChunk(i);
		if (!chunk) continue;
		chunkBuilder->build(chunk);
	}

	delete threadpool;
}

void VoxelWorld::buildChunksTaskSphere(Vector3 cameraPosition, float radius) {
	radius = CHUNK_SIZE * size;
	int x, y, z, nx, ny, nz;
	float dist;
	float lod;
	float r = radius / CHUNK_SIZE;

	Vector3 pos, neighbourPos, chunkMin, neighbourMin;
	Vector3 delta(r, r, r);
	Vector3 cam = fn::toChunkCoords(Vector3(cameraPosition));
	Vector3 min = cam - delta;
	Vector3 max = cam + delta;

	if (max.x < 0 || max.y < 0 || max.z < 0 || min.x >= size || min.y >= size || min.z >= size) return;

	std::shared_ptr<Chunk> chunk, neighbour;
	vector<std::shared_ptr<Chunk>> buildList;
	unordered_set<size_t> buildingChunks;
	auto threadpool = new ThreadPool(8);
	cout << "preparing chunks ..." << endl;

	bpt::ptime start, stop;
	bpt::time_duration dur;
	long ms = 0;

	for (z = std::max(int(min.z), 0); z < std::min(int(max.z), int(size)); z++)
		for (y = std::max(int(min.y), 0); y < std::min(int(max.y), int(size)); y++)
			for (x = std::max(int(min.x), 0); x < std::min(int(max.x), int(size)); x++) {
				pos = Vector3(x, y, z);
				chunkMin = pos * Vector3(CHUNK_SIZE, CHUNK_SIZE, CHUNK_SIZE);
				dist = pos.distance_to(cam);
				if (dist > r) continue;

				//lod = (dist > r * 0.5f) ? 1.f : -1.f;
				lod = OCTREE_LOD;

				//continue;
				chunk = getChunk(x, y, z);
				if (chunk) {
					//chunk = it->second;
					//if (lod != chunk->getLOD()) continue;
					continue;
				}

				//Godot::print(String("pos {0}").format(Array::make(pos)));
				chunk = std::shared_ptr<Chunk>(Chunk::_new());
				chunk->setOffset(chunkMin);
				chunk->setWorld(self);
				setChunk(x, y, z, chunk);

				for (nz = std::max(z - 1, 0); nz <= std::min(z, size - 1); nz++)
					for (ny = std::max(y - 1, 0); ny <= std::min(y, size - 1); ny++)
						for (nx = std::max(x - 1, 0); nx <= std::min(x, size - 1); nx++) {
							neighbourPos = Vector3(nx, ny, nz);
							if (neighbourPos == pos) continue;

							neighbour = getChunk(nx, ny, nz);
							if (!neighbour) continue;

							neighbourMin = neighbourPos * Vector3(CHUNK_SIZE, CHUNK_SIZE, CHUNK_SIZE);
							if (buildingChunks.find(fn::hash(neighbourMin)) != buildingChunks.end()) continue;

							threadpool->submitTask(boost::bind(&VoxelWorld::prepareChunkTask, this, chunk, OCTREE_LOD));
							buildList.push_back(neighbour);
							buildingChunks.emplace(fn::hash(neighbourMin));
						}

				threadpool->submitTask(boost::bind(&VoxelWorld::prepareChunkTask, this, chunk, OCTREE_LOD));
				buildList.push_back(chunk);
				buildingChunks.emplace(fn::hash(chunkMin));
			}

	threadpool->waitUntilFinished();

	stop = bpt::microsec_clock::local_time();
	dur = stop - start;
	ms = dur.total_milliseconds();

	Godot::print(String("chunks prepared in {0} ms").format(Array::make(ms)));

	for (auto chunk : buildList) {
		//Godot::print(String("chunk at {0} building ...").format(Array::make(chunk->getOffset())));
		chunkBuilder->build(chunk);
	}

	delete threadpool;
}

void VoxelWorld::prepareChunkTask(std::shared_ptr<Chunk> chunk, float lod) {
	const Vector3 baseChunkMin = chunk->getOffset();
	try {
		//chunk->buildVolume();
		auto root = BuildOctree(baseChunkMin, CHUNK_SIZE, lod);
		chunk->setRoot(root);
	}
	catch (const std::exception & e) {
		//Godot::print(String("chunk at {0} is empty").format(Array::make(baseChunkMin)));
	}
}

void VoxelWorld::buildQuadTreesTask(Vector3 cameraPosition) {
	quadtreeBuilder->build(quadtrees[0], cameraPosition);
}

void VoxelWorld::buildChunks(Vector3 cameraPosition, float radius) {
	//ThreadPool::get()->submitTask(boost::bind(&VoxelWorld::buildChunksTaskSphere, this, cameraPosition, radius));
	ThreadPool::get()->submitTask(boost::bind(&VoxelWorld::buildChunksTask, this));
	//buildChunksTaskSphere(cameraPosition, radius);
	//buildChunksTask();
}

void VoxelWorld::buildQuadTrees(Vector3 cameraPosition) {
	ThreadPool::get()->submitTask(boost::bind(&VoxelWorld::buildQuadTreesTask, this, cameraPosition));
}
