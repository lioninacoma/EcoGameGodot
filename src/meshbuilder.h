#ifndef MESHBUILDER_H
#define MESHBUILDER_H

#include <String.hpp>
#include <Vector3.hpp>

#include <iostream>

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/thread/thread.hpp>

#include "constants.h"
#include "objectpool.h"
#include "chunk.h"
#include "voxelassetmanager.h"
#include "voxelasset.h"

namespace bpt = boost::posix_time;
using namespace std;

namespace godot {

	class GraphNavNode;

	class MeshBuilder {
	private:
		static const int SOUTH = 0;
		static const int NORTH = 1;
		static const int EAST = 2;
		static const int WEST = 3;
		static const int TOP = 4;
		static const int BOTTOM = 5;

		static ObjectPool<int, BUFFER_SIZE, MAX_CHUNKS_BUILT_ASYNCH>& getMaskPool() {
			static ObjectPool<int, BUFFER_SIZE, MAX_CHUNKS_BUILT_ASYNCH> pool;
			return pool;
		};

		struct NodeUpdateData {
			int bl[3];
			int tl[3]; 
			int tr[3];
			int br[3];
			int side;
			int type;
		};

		float getU(int type);
		float getV(int type);
		int quad(Vector3 offset, int bl[], int tl[], int tr[], int br[], float* vertices, int side, int type, int vertexOffset);
		int createTop(Vector3 offset, int bl[], int tl[], int tr[], int br[], float* vertices, int type, int vertexOffset);
		int createBottom(Vector3 offset, int bl[], int tl[], int tr[], int br[], float* vertices, int type, int vertexOffset);
		int createLeft(Vector3 offset, int bl[], int tl[], int tr[], int br[], float* vertices, int type, int vertexOffset);
		int createRight(Vector3 offset, int bl[], int tl[], int tr[], int br[], float* vertices, int type, int vertexOffset);
		int createFront(Vector3 offset, int bl[], int tl[], int tr[], int br[], float* vertices, int type, int vertexOffset);
		int createBack(Vector3 offset, int bl[], int tl[], int tr[], int br[], float* vertices, int type, int vertexOffset);
		void updateGraphNode(std::shared_ptr<Chunk> chunk, std::shared_ptr<NodeUpdateData> data);
		void updateGraph(std::shared_ptr<Chunk> chunk, vector<std::shared_ptr<NodeUpdateData>> updateData);

		vector<int> buildVertices(std::shared_ptr<VoxelData> volume, std::shared_ptr<Chunk> chunk, Vector3 offset, const int DIMS[3], float** buffers, int buffersLen);
	public:
		MeshBuilder();
		~MeshBuilder();

		vector<int> buildVertices(std::shared_ptr<Chunk> chunk, float** buffers, int buffersLen);
		vector<int> buildVertices(VoxelAsset* asset, Vector3 offset, float** buffers, int buffersLen);
		vector<int> buildVertices(VoxelAssetType type, Vector3 offset, float** buffers, int buffersLen);
	};

}

#endif