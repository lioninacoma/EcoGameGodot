#include "voxelworld.h"
#include "navigator.h"
#include "threadpool.h"
#include "graphnode.h"

#include <boost/exception/diagnostic_information.hpp>
#include <boost/exception_ptr.hpp>

using namespace godot;

static std::shared_ptr<EcoGame> game;

void VoxelWorld::_register_methods() {
	register_method("setVoxel", &VoxelWorld::setVoxel);
	register_method("getVoxel", &VoxelWorld::getVoxel);
	register_method("getVoxelsInArea", &VoxelWorld::getVoxelsInArea);
	register_method("getDisconnectedVoxels", &VoxelWorld::getDisconnectedVoxels);
	register_method("buildSections", &VoxelWorld::buildSections);
	register_method("navigate", &VoxelWorld::navigate);
	register_method("findVoxelsInRange", &VoxelWorld::findVoxelsInRange);
}

VoxelWorld::VoxelWorld() {
	self = std::shared_ptr<VoxelWorld>(this);
	chunkBuilder = std::shared_ptr<ChunkBuilder>(new ChunkBuilder(self));
	navigator = std::shared_ptr<Navigator>(new Navigator(self));
	sections = new std::shared_ptr<Section>[SECTIONS_LEN * sizeof(*sections)];

	memset(sections, 0, SECTIONS_LEN * sizeof(*sections));
}

VoxelWorld::~VoxelWorld() {
	//delete[] sections;
	chunkBuilder.reset();
}

void VoxelWorld::_init() {
	// initialize any variables here
}

std::shared_ptr<Section> VoxelWorld::intersection(int x, int y, int z) {
	std::shared_ptr<Section> section = NULL;

	try {
		//Godot::print(String("Section: {0}").format(Array::make(Vector3(x, y, z))));
		std::shared_ptr<Section> section = getSection(x, z);
		if (!section) return NULL;
	}
	catch (const std::exception & e) {
		std::cerr << boost::diagnostic_information(e);
	}

	return section;
}

vector<std::shared_ptr<Section>> VoxelWorld::getSectionsRay(Vector3 from, Vector3 to) {
	from = fn::toSectionCoords(from);
	to = fn::toSectionCoords(to);
	vector<std::shared_ptr<Section>> sections;

	try {
		vector<std::shared_ptr<Section>> list;
		boost::function<std::shared_ptr<Section>(int, int, int)> intersection(boost::bind(&VoxelWorld::intersection, this, _1, _2, _3));
		sections = Intersection::get<std::shared_ptr<Section>>(from, to, false, intersection, list);
	}
	catch (const std::exception & e) {
		std::cerr << boost::diagnostic_information(e);
	}

	return sections;
}

vector<std::shared_ptr<Chunk>> VoxelWorld::getChunksRay(Vector3 from, Vector3 to) {
	vector<std::shared_ptr<Chunk>> list;
	vector<std::shared_ptr<Chunk>> chunks;

	try {
		vector<std::shared_ptr<Section>> sections = getSectionsRay(from, to);

		for (auto section : sections) {
			chunks = section->getChunksRay(from, to);
			list.insert(list.end(), chunks.begin(), chunks.end());
		}
	}
	catch (const std::exception & e) {
		std::cerr << boost::diagnostic_information(e);
	}

	return list;
}

vector<std::shared_ptr<Chunk>> VoxelWorld::getChunksInRange(Vector3 center, float radius) {
	vector<std::shared_ptr<Chunk>> list;
	int x, z, i;
	int dist = radius / (SECTION_SIZE * CHUNK_SIZE_X);
	Vector3 p = Vector3(center.x, 0, center.z);
	Vector3 cc = Vector3(CHUNK_SIZE_X / 2, 0, CHUNK_SIZE_Z / 2);
	std::shared_ptr<Section> section;

	try {
		p = fn::toSectionCoords(p);

		for (z = -dist + (int)p.z; z <= dist + (int)p.z; z++) {
			for (x = -dist + (int)p.x; x <= dist + (int)p.x; x++) {
				if (x < 0 || z < 0 || x >= SECTIONS_SIZE || z >= SECTIONS_SIZE) continue;
				section = getSection(x, z);

				if (!section) continue;

				for (i = 0; i < SECTION_CHUNKS_LEN; i++) {
					auto chunk = section->getChunk(i);

					if (!chunk || (chunk->getOffset() + cc).distance_to(center) > radius) continue;
					list.push_back(section->getChunk(i));
				}
			}
		}
	}
	catch (const std::exception & e) {
		std::cerr << boost::diagnostic_information(e);
	}

	return list;
}

std::shared_ptr<Chunk> VoxelWorld::getChunk(Vector3 position) {
	std::shared_ptr<Section> section;
	std::shared_ptr<Chunk> chunk;
	int x, z, cx, cz;
	Vector2 chunkOffset, sectionOffset;

	sectionOffset.x = position.x;
	sectionOffset.y = position.z;
	sectionOffset = fn::toSectionCoords(sectionOffset);
	x = (int)sectionOffset.x;
	z = (int)sectionOffset.y;
	section = getSection(x, z);

	if (!section) return NULL;

	chunkOffset.x = position.x;
	chunkOffset.y = position.z;
	chunkOffset = fn::toChunkCoords(chunkOffset);
	cx = (int)(chunkOffset.x - sectionOffset.x);
	cz = (int)(chunkOffset.y - sectionOffset.y);
	chunk = section->getChunk(cx, cz);

	if (!chunk) return NULL;
	return chunk;
}

void VoxelWorld::navigate(Vector3 startV, Vector3 goalV, int actorInstanceId) {
	ThreadPool::getNav()->submitTask(boost::bind(&VoxelWorld::navigateTask, this, startV, goalV, actorInstanceId));
}

void VoxelWorld::navigateTask(Vector3 startV, Vector3 goalV, int actorInstanceId) {
	navigator->navigate(startV, goalV, actorInstanceId);
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
					s /= 40.0;
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
	int voxel = 0;
	try {
		Vector3 p = position;
		std::shared_ptr<Section> section;
		p = fn::toSectionCoords(p);
		section = getSection(p.x, p.z);
		if (!section) return 0;
		voxel = section->getVoxel(position);
	}
	catch (const std::exception & e) {
		std::cerr << boost::diagnostic_information(e);
	}
	return voxel;
}

std::shared_ptr<GraphNavNode> VoxelWorld::getNode(Vector3 position) {
	std::shared_ptr<GraphNavNode> node;
	try {
		Vector3 p = position;
		std::shared_ptr<Section> section;
		p = fn::toSectionCoords(p);
		section = getSection(p.x, p.z);
		if (!section) return NULL;
		node = section->getNode(position);
	}
	catch (const std::exception & e) {
		std::cerr << boost::diagnostic_information(e);
	}
	return node;
}

void VoxelWorld::setSection(int x, int z, std::shared_ptr<Section> section) {
	int i = fn::fi2(x, z, SECTIONS_SIZE);
	return setSection(i, section);
}

void VoxelWorld::setSection(int i, std::shared_ptr<Section> section) {
	try {
		boost::unique_lock<boost::mutex> lock(SECTIONS_MUTEX);
		if (i < 0 || i >= SECTIONS_LEN) return;
		sections[i] = section;
	}
	catch (const std::exception & e) {
		std::cerr << boost::diagnostic_information(e);
	}
}

std::shared_ptr<Section> VoxelWorld::getSection(int x, int z) {
	int i = fn::fi2(x, z, SECTIONS_SIZE);
	return getSection(i);
}

std::shared_ptr<Section> VoxelWorld::getSection(int i) {
	boost::unique_lock<boost::mutex> lock(SECTIONS_MUTEX);
	std::shared_ptr<Section> section;
	try {
		if (i < 0 || i >= SECTIONS_LEN) return NULL;
		section = sections[i];
	}
	catch (const std::exception & e) {
		std::cerr << boost::diagnostic_information(e);
	}
	return section;
}

void VoxelWorld::buildSections(Vector3 pos, float d, int maxSectionsBuilt) {
	int x, z;
	int dist = 0;
	int sectionsBuilt = 0;
	int it = 0;
	int maxIt = (int)((Math_PI * d * d) / (CHUNK_SIZE_X * SECTION_CHUNKS_LEN));
	std::shared_ptr<Section> section;
	Vector3 p = Vector3(pos.x, 0, pos.z);

	p = fn::toSectionCoords(p);

	while (sectionsBuilt < maxSectionsBuilt) {
		for (z = -dist + (int)p.z; z <= dist + (int)p.z; z++) {
			for (x = -dist + (int)p.x; x <= dist + (int)p.x; x++) {
				if (++it >= maxIt) return;
				if (x < 0 || z < 0 || x >= SECTIONS_SIZE || z >= SECTIONS_SIZE) continue;
				if (p.distance_to(Vector3(x, 0, z)) > dist + 1) continue;
				if (sections[fn::fi2(x, z, SECTIONS_SIZE)]) continue;

				Godot::print(String("section {0} building ...").format(Array::make(Vector2(x * SECTION_SIZE, z * SECTION_SIZE))));
				section = std::shared_ptr<Section>(new Section(self, Vector2(x * SECTION_SIZE, z * SECTION_SIZE)));
				setSection(x, z, section);
				ThreadPool::get()->submitTask(boost::bind(&VoxelWorld::buildSection, this, section));

				if (++sectionsBuilt >= maxSectionsBuilt) return;
			}
		}
		dist++;
	}
}

void VoxelWorld::buildSection(std::shared_ptr<Section> section) {
	section->build(chunkBuilder);
}

PoolVector3Array VoxelWorld::findVoxelsInRange(Vector3 startV, float radius, int voxel) {
	bpt::ptime startTime, stopTime;
	bpt::time_duration dur;
	long ms = 0;

	startTime = bpt::microsec_clock::local_time();

	PoolVector3Array voxels;
	try {
		std::shared_ptr<Section> section;

		Vector3 start = startV - Vector3(radius, radius, radius);
		Vector3 end = startV + Vector3(radius, radius, radius);
		Vector3 tl = start;
		Vector3 br = end;

		tl = fn::toChunkCoords(tl);
		br = fn::toChunkCoords(br);

		Vector2 offset = Vector2(tl.x, tl.z);
		int sectionSize = max(abs(br.x - tl.x), abs(br.z - tl.z)) + 1;

		section = std::shared_ptr<Section>(new Section(self, offset, sectionSize));
		section->fill(SECTION_SIZE);
		voxels = section->findVoxelsInRange(startV, radius, voxel);
		section.reset();
	}
	catch (const std::exception & e) {
		std::cerr << boost::diagnostic_information(e);
	}

	stopTime = bpt::microsec_clock::local_time();
	dur = stopTime - startTime;
	ms = dur.total_milliseconds();

	//Godot::print(String("found voxels of type {0} at {1} in range {2} in {3} ms").format(Array::make(voxel, startV, radius, ms)));

	return voxels;
}

Array VoxelWorld::getDisconnectedVoxels(Vector3 position, float radius) {
	bpt::ptime startTime, stopTime;
	bpt::time_duration dur;
	long ms = 0;

	startTime = bpt::microsec_clock::local_time();

	Array voxels;
	std::shared_ptr<Section> section;

	Vector3 start = position - Vector3(radius, radius, radius);
	Vector3 end = position + Vector3(radius, radius, radius);
	Vector3 tl = start;
	Vector3 br = end;

	try {
		tl = fn::toChunkCoords(tl);
		br = fn::toChunkCoords(br);

		Vector2 offset = Vector2(tl.x, tl.z);
		int sectionSize = max(abs(br.x - tl.x), abs(br.z - tl.z)) + 1;

		section = std::shared_ptr<Section>(new Section(self, offset, sectionSize));
		section->fill(SECTION_SIZE);
		voxels = section->getDisconnectedVoxels(position, start, end);
		section.reset();
	}
	catch (const std::exception & e) {
		std::cerr << boost::diagnostic_information(e);
	}

	stopTime = bpt::microsec_clock::local_time();
	dur = stopTime - startTime;
	ms = dur.total_milliseconds();

	//Godot::print(String("disconnected voxels at {0} in range {1} got in {2} ms").format(Array::make(position, radius, ms)));

	return voxels;
}

PoolVector3Array VoxelWorld::getVoxelsInArea(Vector3 start, Vector3 end, int voxel) {
	bpt::ptime startTime, stopTime;
	bpt::time_duration dur;
	long ms = 0;

	startTime = bpt::microsec_clock::local_time();

	int i, j;
	PoolVector3Array currentVoxels, result;
	unordered_map<size_t, Vector3> voxels;
	std::shared_ptr<Section> section;
	Vector3 tl = start;
	Vector3 br = end;
	Vector3 current;
	Vector2 start2 = Vector2(start.x, start.z);
	Vector2 end2 = Vector2(end.x, end.z);

	try {
		tl = fn::toChunkCoords(tl);
		br = fn::toChunkCoords(br);

		Vector2 offset = Vector2(tl.x, tl.z);
		int sectionSize = max(abs(br.x - tl.x), abs(br.z - tl.z)) + 1;
		int sectionChunksLen = sectionSize * sectionSize;

		section = std::shared_ptr<Section>(new Section(self, offset, sectionSize));
		section->fill(SECTION_SIZE);

		for (i = 0; i < sectionChunksLen; i++) {
			auto chunk = section->getChunk(i);
			if (!chunk) continue;

			std::function<void(std::pair<size_t, std::shared_ptr<GraphNavNode>>)> nodeFn = [&](auto next) {
				currentVoxels = chunk->getReachableVoxelsOfType(*next.second->getPoint(), voxel);

				for (j = 0; j < currentVoxels.size(); j++) {
					current = currentVoxels[j];
					if (!Intersection::isPointInRectangle(start2, end2, Vector2(current.x + 0.5, current.z + 0.5))) continue;
					voxels.emplace(fn::hash(current), current);
				}
			};
			chunk->forEachNode(nodeFn);
		}

		section.reset();

		for (auto v : voxels)
			result.push_back(v.second);
	}
	catch (const std::exception & e) {
		std::cerr << boost::diagnostic_information(e);
	}

	stopTime = bpt::microsec_clock::local_time();
	dur = stopTime - startTime;
	ms = dur.total_milliseconds();

	//Godot::print(String("voxels of type {0} from {1} to {2} got in {3} ms").format(Array::make(voxel, start, end, ms)));
	return result;
}
