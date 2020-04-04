#ifndef VOXELWORLD_H
#define VOXELWORLD_H

#include <Godot.hpp>
#include <SceneTree.hpp>
#include <ViewPort.hpp>
#include <Spatial.hpp>
#include <String.hpp>
#include <Array.hpp>
#include <Vector3.hpp>

#include <limits>
#include <iostream>
#include <unordered_map> 
#include <iterator>
#include <exception>
#include <stdexcept>
#include <boost/thread/mutex.hpp>

#include "constants.h"
#include "fn.h"
#include "area.h"
#include "chunkbuilder.h"
#include "section.h"
#include "meshbuilder.h"
#include "intersection.h"
#include "voxel.h"
#include "navigator.h"

using namespace std;

namespace godot {
	class GraphNavNode;

	class VoxelWorld : public Spatial {
		GODOT_CLASS(VoxelWorld, Spatial)

	private:
		std::shared_ptr<VoxelWorld> self;
		std::shared_ptr<Section>* sections;
		std::shared_ptr<Navigator> navigator;
		std::shared_ptr<ChunkBuilder> chunkBuilder;

		boost::mutex SECTIONS_MUTEX;

		void buildSection(std::shared_ptr<Section> section);
		void navigateTask(Vector3 startV, Vector3 goalV, int actorInstanceId);
	public:
		static void _register_methods();

		VoxelWorld();
		~VoxelWorld();

		void _init();

		std::shared_ptr<Section> intersection(int x, int y, int z);
		vector<std::shared_ptr<Section>> getSectionsRay(Vector3 from, Vector3 to);
		vector<std::shared_ptr<Chunk>> getChunksRay(Vector3 from, Vector3 to);
		vector<std::shared_ptr<Chunk>> getChunksInRange(Vector3 center, float radius);
		std::shared_ptr<Chunk> getChunk(Vector3 position);
		std::shared_ptr<Section> getSection(int x, int z);
		std::shared_ptr<Section> getSection(int i);
		std::shared_ptr<GraphNavNode> getNode(Vector3 position);
		Array getDisconnectedVoxels(Vector3 position, float radius);
		PoolVector3Array getVoxelsInArea(Vector3 start, Vector3 end, int voxel);
		PoolVector3Array findVoxelsInRange(Vector3 startV, float radius, int voxel);
		int getVoxel(Vector3 position);

		void setSection(int x, int z, std::shared_ptr<Section> section);
		void setSection(int i, std::shared_ptr<Section> section);
		void buildSections(Vector3 center, float radius, int maxSectionsBuilt);
		void setVoxel(Vector3 position, float radius, bool set);
		std::shared_ptr<Navigator> getNavigator() {
			return navigator;
		};
		void navigate(Vector3 startV, Vector3 goalV, int actorInstanceId);
	};
}

#endif