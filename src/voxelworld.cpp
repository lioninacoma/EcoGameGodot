#include "voxelworld.h"
#include "navigator.h"
#include "threadpool.h"
#include "graphnode.h"

#include <boost/exception/diagnostic_information.hpp>
#include <boost/exception_ptr.hpp>

using namespace godot;

void VoxelWorld::_register_methods() {
	register_method("setDimensions", &VoxelWorld::setDimensions);
	register_method("getWidth", &VoxelWorld::getWidth);
	register_method("getDepth", &VoxelWorld::getDepth);
	register_method("setVoxel", &VoxelWorld::setVoxel);
	register_method("getVoxel", &VoxelWorld::getVoxel);
	register_method("buildChunks", &VoxelWorld::buildChunks);
	register_method("navigate", &VoxelWorld::navigate);
	register_method("setIsVoxelFn", &VoxelWorld::setIsVoxelFn);
	register_method("_notification", &VoxelWorld::_notification);
}

VoxelWorld::VoxelWorld() {
	VoxelWorld::width = 16;
	VoxelWorld::depth = 16;

	self = std::shared_ptr<VoxelWorld>(this);
	chunkBuilder = std::make_unique<ChunkBuilder>(self);
	navigator = std::make_unique<Navigator>(self);
	chunks = vector<std::shared_ptr<Chunk>>(size_t(width * depth));
}

VoxelWorld::~VoxelWorld() {
	chunkBuilder.reset();
}

void VoxelWorld::_init() {

}

std::shared_ptr<Chunk> VoxelWorld::intersection(int x, int y, int z) {
	std::shared_ptr<Chunk> chunk = getChunk(x, z);
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

std::shared_ptr<Chunk> VoxelWorld::getChunk(int x, int z) {
	if (x < 0 || x >= width || z < 0 || z >= depth) return NULL;
	int i = fn::fi2(x, z, width);
	return getChunk(i);
}

std::shared_ptr<Chunk> VoxelWorld::getChunk(int i) {
	boost::shared_lock<boost::shared_mutex> lock(CHUNKS_MUTEX);
	if (i < 0 || i >= width * depth) return NULL;
	return chunks.at(i);
}

std::shared_ptr<Chunk> VoxelWorld::getChunk(Vector3 position) {
	//Godot::print(String("get chunk at: {0}").format(Array::make(position)));
	if (position.x < 0 || position.x >= width * CHUNK_SIZE_X
		|| position.z < 0 || position.z >= depth * CHUNK_SIZE_Z)
		return NULL;

	int cx, cz;
	Vector3 chunkOffset = position;
	chunkOffset = fn::toChunkCoords(chunkOffset);
	cx = (int)chunkOffset.x;
	cz = (int)chunkOffset.z;
	auto chunk = getChunk(cx, cz);

	if (!chunk) return NULL;
	return chunk;
}

int VoxelWorld::getWidth() {
	return width;
}

int VoxelWorld::getDepth() {
	return depth;
}

void VoxelWorld::_notification(const int64_t what) {
	if (what == Node::NOTIFICATION_READY) {}
}

void VoxelWorld::setDimensions(Vector2 dimensions) {
	VoxelWorld::width = dimensions.x;
	VoxelWorld::depth = dimensions.y;
	// TODO: NOT WORKING? INITIAL WIDTH AND DEPTH ARE SET
	//chunks.resize(width * depth);
	//chunks = vector<std::shared_ptr<Chunk>>(size_t(dimensions.x * dimensions.y));
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

					updatingChunks.emplace(fn::hash(chunk->getOffset()), chunk);

					if ((x + 1) % CHUNK_SIZE_X == 0) {
						neighbourPosition = currentPosition;
						neighbourPosition.x = x + 1;
						neighbour = getChunk(neighbourPosition);
						if (neighbour) updatingChunks.emplace(fn::hash(neighbour->getOffset()), neighbour);
					}

					if (((x - 1) + CHUNK_SIZE_X) % CHUNK_SIZE_X == 0) {
						neighbourPosition = currentPosition;
						neighbourPosition.x = x - 1;
						neighbour = getChunk(neighbourPosition);
						if (neighbour) updatingChunks.emplace(fn::hash(neighbour->getOffset()), neighbour);
					}

					if ((z + 1) % CHUNK_SIZE_Z == 0) {
						neighbourPosition = currentPosition;
						neighbourPosition.z = z + 1;
						neighbour = getChunk(neighbourPosition);
						if (neighbour) updatingChunks.emplace(fn::hash(neighbour->getOffset()), neighbour);
					}

					if (((z - 1) + CHUNK_SIZE_Z) % CHUNK_SIZE_Z == 0) {
						neighbourPosition = currentPosition;
						neighbourPosition.z = z - 1;
						neighbour = getChunk(neighbourPosition);
						if (neighbour) updatingChunks.emplace(fn::hash(neighbour->getOffset()), neighbour);
					}

					s = 1.0 - (currentPosition.distance_to(position) / radius);
					s /= 100.0;
					chunk->setVoxel(
						x % CHUNK_SIZE_X,
						y % CHUNK_SIZE_Y,
						z % CHUNK_SIZE_Z, s * m);
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
		x % CHUNK_SIZE_X,
		y % CHUNK_SIZE_Y,
		z % CHUNK_SIZE_Z);
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

void VoxelWorld::setChunk(int x, int z, std::shared_ptr<Chunk> chunk) {
	int i = fn::fi2(x, z, width);
	return setChunk(i, chunk);
}

void VoxelWorld::setChunk(int i, std::shared_ptr<Chunk> chunk) {
	boost::unique_lock<boost::shared_mutex> lock(CHUNKS_MUTEX);
	if (i < 0 || i >= width * depth) return;
	chunks.insert(chunks.begin() + i, chunk);
}

void VoxelWorld::buildChunksTask(std::shared_ptr<VoxelWorld> world) {
	if (isVoxelFn == NULL) return;

	int x, y, i;
	std::shared_ptr<Chunk> chunk;

	float world_width_2 = CHUNK_SIZE_X * width / 2;
	float world_height_2 = CHUNK_SIZE_Y / 2;
	float world_depth_2 = CHUNK_SIZE_Z * depth / 2;
	Vector3 cog = Vector3(world_width_2, world_height_2, world_depth_2);

	for (y = 0; y < depth; y++) {
		for (x = 0; x < width; x++) {
			chunk = std::shared_ptr<Chunk>(Chunk::_new());
			chunk->setOffset(Vector3(x * CHUNK_SIZE_X, 0, y * CHUNK_SIZE_Z));
			chunk->setCenterOfGravity(cog);
			chunk->setWorld(world);
			chunk->setIsVoxelFn(isVoxelFn);
			setChunk(x, y, chunk);
			chunk->buildVolume();
		}
	}

	for (i = 0; i < width * depth; i++) {
		chunk = getChunk(i);
		if (!chunk) continue;
		chunkBuilder->build(chunk);
	}
}

void VoxelWorld::buildChunks() {
	ThreadPool::get()->submitTask(boost::bind(&VoxelWorld::buildChunksTask, this, self));
}
