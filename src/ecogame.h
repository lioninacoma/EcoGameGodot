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
#include <iomanip>
#include <set> 
#include <iterator> 

#include "constants.h"
#include "fn.h"
#include "area.h"
#include "chunkbuilder.h"
#include "voxelassetmanager.h"
#include "section.h"
#include "navigator.h"
#include "meshbuilder.h"

using namespace std;

namespace godot {
	class EcoGame : public Node {
		GODOT_CLASS(EcoGame, Node)

	private:
		Section** sections;
		ChunkBuilder* chunkBuilder;

		void buildSection(Section* section, Node* game);
		void updateGraphTask(Chunk* chunk, Node* game);
	public:
		static void _register_methods();

		EcoGame();
		~EcoGame();

		void _init();

		void buildSections(Vector3 center, float radius, int maxSectionsBuilt);
		void buildChunk(Variant vChunk);
		void setVoxel(Vector3 position, int voxel);
		void addVoxelAsset(Vector3 start, int type);
		Array buildVoxelAsset(int type);
		bool voxelAssetFits(Vector3 start, int type);
		PoolVector3Array navigate(Vector3 startV, Vector3 goalV);
		PoolVector3Array navigateToClosestVoxel(Vector3 startV, int voxel);
		PoolVector3Array findVoxelsInRange(Vector3 startV, float radius, int voxel);
		void updateGraph(Variant vChunk);
		void updateGraphs(Vector3 center, float radius);
		int getVoxel(Vector3 position);
	};

}

#endif