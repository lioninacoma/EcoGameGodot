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

static int WORLD_Y = 2;

VoxelWorld::VoxelWorld() {
	VoxelWorld::size = 16;
	WORLD_Y = size;

	self = std::shared_ptr<VoxelWorld>(this);
	chunkBuilder = std::make_unique<ChunkBuilder>(self);
	quadtreeBuilder = std::make_unique<QuadTreeBuilder>(self);
	navigator = std::make_unique<Navigator>(self);
	chunks = vector<std::shared_ptr<Chunk>>(size_t(size * WORLD_Y * size));
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
	if (x < 0 || x >= size || y < 0 || y >= WORLD_Y || z < 0 || z >= size) return NULL;
	int i = fn::fi3(x, y, z, size, WORLD_Y);
	return getChunk(i);
}

std::shared_ptr<Chunk> VoxelWorld::getChunk(int i) {
	boost::shared_lock<boost::shared_mutex> lock(CHUNKS_MUTEX);
	if (i < 0 || i >= size * WORLD_Y * size) return NULL;
	return chunks.at(i);
}

std::shared_ptr<Chunk> VoxelWorld::getChunk(Vector3 position) {
	//Godot::print(String("get chunk at: {0}").format(Array::make(position)));
	if (position.x < 0 || position.x >= size * CHUNK_SIZE
		|| position.y < 0 || position.y >= WORLD_Y * CHUNK_SIZE
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
	chunks = vector<std::shared_ptr<Chunk>>(size_t(size * size * size));
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
				BuildOctree(baseChunkMin, CHUNK_SIZE, OCTREE_LOD, chunk.second);
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
	int i = fn::fi3(x, y, z, size, WORLD_Y);
	return setChunk(i, chunk);
}

void VoxelWorld::setChunk(int i, std::shared_ptr<Chunk> chunk) {
	boost::unique_lock<boost::shared_mutex> lock(CHUNKS_MUTEX);
	if (i < 0 || i >= size * WORLD_Y * size) return;
	chunks.insert(chunks.begin() + i, chunk);
}

void VoxelWorld::buildChunksTask(std::shared_ptr<VoxelWorld> world) {
	if (isVoxelFn == NULL) return;

	int x, y, z, i;
	std::shared_ptr<Chunk> chunk;
	vector<std::shared_ptr<boost::thread>> threads;

	for (z = 0; z < size; z++) {
		for (y = 0; y < WORLD_Y; y++) {
			for (x = 0; x < size; x++) {
				const Vector3 baseChunkMin = Vector3(x * CHUNK_SIZE, y * CHUNK_SIZE, z * CHUNK_SIZE);
				chunk = std::shared_ptr<Chunk>(Chunk::_new());
				chunk->setOffset(baseChunkMin);
				chunk->setWorld(world);
				setChunk(x, y, z, chunk);
				auto thread = std::make_shared<boost::thread>(&VoxelWorld::prepareChunkTask, this, chunk);
				threads.push_back(thread);
			}
		}
	}

	for (auto thread : threads) {
		thread->join();
	}

	for (i = 0; i < size * WORLD_Y * size; i++) {
		chunk = getChunk(i);
		if (!chunk) continue;
		chunkBuilder->build(chunk);
	}
}

void VoxelWorld::prepareChunkTask(std::shared_ptr<Chunk> chunk) {
	const Vector3 baseChunkMin = chunk->getOffset();
	try {
		//chunk->buildVolume();
		BuildOctree(baseChunkMin, CHUNK_SIZE, OCTREE_LOD, chunk);
	}
	catch (const std::exception & e) {
		//Godot::print(String("chunk at {0} is empty").format(Array::make(baseChunkMin)));
	}
}

void VoxelWorld::buildQuadTreesTask(std::shared_ptr<VoxelWorld> world, Vector3 cameraPosition) {
	quadtreeBuilder->build(quadtrees[0], cameraPosition);
}

void VoxelWorld::buildChunks() {
	ThreadPool::get()->submitTask(boost::bind(&VoxelWorld::buildChunksTask, this, self));
}

void VoxelWorld::buildQuadTrees(Vector3 cameraPosition) {
	ThreadPool::get()->submitTask(boost::bind(&VoxelWorld::buildQuadTreesTask, this, self, cameraPosition));
}
