#ifndef ECOGAME_H
#define ECOGAME_H

#include <Godot.hpp>
#include <SceneTree.hpp>
#include <ViewPort.hpp>
#include <Node.hpp>
#include <String.hpp>
#include <Array.hpp>
#include <Vector3.hpp>

#include <iostream>
#include <unordered_map> 
#include <iterator> 

#include <boost/atomic.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_types.hpp>

#include "constants.h"
#include "fn.h"
#include "area.h"
#include "chunkbuilder.h"
#include "voxelassetmanager.h"
#include "section.h"
#include "meshbuilder.h"
#include "graphnode.h"
#include "intersection.h"

using namespace std;

namespace godot {
	class EcoGame : public Node {
		GODOT_CLASS(EcoGame, Node)

	private:
		boost::shared_ptr<Section>* sections;
		boost::shared_ptr<ChunkBuilder> chunkBuilder;

		boost::shared_timed_mutex SECTIONS_MUTEX;

		void buildSection(boost::shared_ptr<Section> section, Node* game);
		void updateGraphTask(boost::shared_ptr<Chunk> chunk, Node* game);
		void navigateTask(Vector3 startV, Vector3 goalV, int actorInstanceId, Node* game);
		void navigateToClosestVoxelTask(Vector3 startV, int voxel, int actorInstanceId, Node* game, EcoGame* lib);
	public:
		static void _register_methods();

		EcoGame();
		~EcoGame();

		void _init();

		boost::shared_ptr<Section> intersection(int x, int y, int z);
		vector<boost::shared_ptr<Section>> getSectionsRay(Vector3 from, Vector3 to);
		vector<boost::shared_ptr<Chunk>> getChunksRay(Vector3 from, Vector3 to);
		vector<boost::shared_ptr<Chunk>> getChunksInRange(Vector3 center, float radius);
		PoolVector3Array getDisconnectedVoxels(Vector3 center, float radius);
		PoolVector3Array getVoxelsInArea(Vector3 start, Vector3 end, int voxel);
		void setSection(int x, int z, boost::shared_ptr<Section> section);
		void setSection(int i, boost::shared_ptr<Section> section);
		boost::shared_ptr<Section> getSection(int x, int z);
		boost::shared_ptr<Section> getSection(int i);
		void buildSections(Vector3 center, float radius, int maxSectionsBuilt);
		void buildChunk(Variant vChunk);
		void setVoxel(Vector3 position, int voxel);
		void addVoxelAsset(Vector3 start, int type);
		Array buildVoxelAsset(int type);
		bool voxelAssetFits(Vector3 start, int type);
		void navigate(Vector3 startV, Vector3 goalV, int actorInstanceId);
		void navigateToClosestVoxel(Vector3 startV, int voxel, int actorInstanceId);
		PoolVector3Array findVoxelsInRange(Vector3 startV, float radius, int voxel);
		void updateGraph(Variant vChunk);
		void updateGraphs(Vector3 center, float radius);
		int getVoxel(Vector3 position);
		boost::shared_ptr<GraphNode> getNode(Vector3 position);
	};

}

#endif