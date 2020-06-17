#ifndef CHUNK_H
#define CHUNK_H

#include <Godot.hpp>
#include <Node.hpp>
#include <MeshInstance.hpp>
#include <OpenSimplexNoise.hpp>
#include <FuncRef.hpp>

#include <vector>
#include <unordered_map>
#include <limits>
#include <atomic>
#include <shared_mutex>
#include <boost/thread/mutex.hpp>

#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/exception/diagnostic_information.hpp> 
#include <boost/exception_ptr.hpp> 

#include "constants.h"
#include "fn.h"
#include "intersection.h"
#include "voxel.h"
#include "voxeldata.h"

using namespace std;

namespace godot {

	class GraphNode;
	class GraphEdge;
	class VoxelWorld;
	class EcoGame;

	class Chunk : public Node {
		GODOT_CLASS(Chunk, Node)
	private:
		std::atomic<int> gridSize;
		std::atomic<int> amountNodes = 0;
		std::atomic<int> meshInstanceId = 0;
		std::atomic<bool> navigatable = false;
		std::atomic<bool> building = false;
		std::atomic<bool> volumeBuilt = false;
		
		Vector3 offset;
		Ref<FuncRef> isVoxelFn;

		std::shared_ptr<VoxelWorld> world;
		std::unique_ptr<VoxelData> volume;
		unordered_map<size_t, std::shared_ptr<GraphNode>> nodes;
		
		OpenSimplexNoise* noise;

		std::shared_mutex CHUNK_NODES_MUTEX;
		std::shared_mutex VERTICES_MUTEX;
		std::shared_mutex FACES_MUTEX;

		Voxel* intersection(int x, int y, int z);
	public:
		static void _register_methods();

		Chunk() : Chunk(Vector3(0, 0, 0)) {};
		Chunk(Vector3 offset);
		~Chunk();

		void _init(); // our initializer called by Godot
		
		Vector3 getOffset();
		bool isNavigatable();
		bool isBuilding();
		int getMeshInstanceId();
		int getGridSize();
		int getAmountNodes();
		float isVoxel(int x, int y, int z);
		float getVoxel(int x, int y, int z);
		Voxel* getVoxelRay(Vector3 from, Vector3 to);
		std::shared_ptr<GraphNode> getNode(size_t hash);
		std::shared_ptr<GraphNode> fetchNode(Vector3 position);
		void forEachNode(std::function<void(std::pair<size_t, std::shared_ptr<GraphNode>>)> func);

		void setWorld(std::shared_ptr<VoxelWorld> world);
		void setOffset(Vector3 offset);
		void setNavigatable(bool navigatable);
		void setBuilding(bool building);
		void setMeshInstanceId(int meshInstanceId);
		void setIsVoxelFn(Ref<FuncRef> fnRef);
		void setVoxel(int x, int y, int z, float v);
		void buildVolume();
		void addNode(std::shared_ptr<GraphNode> node);
		void removeNode(std::shared_ptr<GraphNode> node);
		void addFaceNodes(Vector3 a, Vector3 b, Vector3 c, Vector3 normal);
		void addEdge(std::shared_ptr<GraphNode> a, std::shared_ptr<GraphNode> b, float cost);
		std::shared_ptr<GraphNode> fetchOrCreateNode(Vector3 position, Vector3 normal);
	};

}

#endif