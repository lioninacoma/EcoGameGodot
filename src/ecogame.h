#ifndef ECOGAME_H
#define ECOGAME_H

#include <Godot.hpp>
#include <SceneTree.hpp>
#include <ViewPort.hpp>
#include <Node.hpp>
#include <String.hpp>
#include <Array.hpp>
#include <Vector3.hpp>

#include <limits>
#include <iostream>
#include <unordered_map> 
#include <iterator>
#include <exception>
#include <stdexcept>
#include <shared_mutex>
#include <mutex>

#include "constants.h"
#include "fn.h"
#include "area.h"
#include "chunkbuilder.h"
#include "voxelassetmanager.h"
#include "section.h"
#include "meshbuilder.h"
#include "graphnode.h"
#include "intersection.h"
#include "voxel.h"

using namespace std;

namespace godot {
	class EcoGame : public Node {
		GODOT_CLASS(EcoGame, Node)

	private:
		std::shared_ptr<Section>* sections;
		std::shared_ptr<ChunkBuilder> chunkBuilder;

		std::shared_timed_mutex SECTIONS_MUTEX;

		void buildSection(std::shared_ptr<Section> section, Node* game);
		void updateGraphTask(std::shared_ptr<Chunk> chunk, Node* game);
		void navigateTask(Vector3 startV, Vector3 goalV, int actorInstanceId, Node* game);
		void navigateToClosestVoxelTask(Vector3 startV, int voxel, int actorInstanceId, Node* game, EcoGame* lib);
	public:
		static void _register_methods();

		EcoGame();
		~EcoGame();

		void _init();

		std::shared_ptr<Section> intersection(int x, int y, int z);
		vector<std::shared_ptr<Section>> getSectionsRay(Vector3 from, Vector3 to);
		vector<std::shared_ptr<Chunk>> getChunksRay(Vector3 from, Vector3 to);
		vector<std::shared_ptr<Chunk>> getChunksInRange(Vector3 center, float radius);
		Array getDisconnectedVoxels(Vector3 position, float radius);
		PoolVector3Array getVoxelsInArea(Vector3 start, Vector3 end, int voxel);
		void setSection(int x, int z, std::shared_ptr<Section> section);
		void setSection(int i, std::shared_ptr<Section> section);
		std::shared_ptr<Section> getSection(int x, int z);
		std::shared_ptr<Section> getSection(int i);
		void buildSections(Vector3 center, float radius, int maxSectionsBuilt);
		void buildChunk(Variant vChunk);
		void setVoxel(Vector3 position, int voxel);
		void addVoxelAsset(Vector3 start, int type);
		Array buildVoxelAssetByType(int type);
		Array buildVoxelAssetByVolume(Array volume);
		Array buildVoxelAsset(VoxelAsset* voxelAsset);
		bool voxelAssetFits(Vector3 start, int type);
		void navigate(Vector3 startV, Vector3 goalV, int actorInstanceId);
		void navigateToClosestVoxel(Vector3 startV, int voxel, int actorInstanceId);
		PoolVector3Array findVoxelsInRange(Vector3 startV, float radius, int voxel);
		void updateGraph(Variant vChunk);
		void updateGraphs(Vector3 center, float radius);
		int getVoxel(Vector3 position);
		std::shared_ptr<GraphNode> getNode(Vector3 position);
	};

}

#endif