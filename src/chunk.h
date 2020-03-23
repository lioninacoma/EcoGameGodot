#ifndef CHUNK_H
#define CHUNK_H

#include <Godot.hpp>
#include <Node.hpp>
#include <MeshInstance.hpp>
#include <OpenSimplexNoise.hpp>

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
#include "graphnode.h"

using namespace std;

namespace godot {

	class GraphNavNode;
	class Section;

	class Chunk : public Node {
		GODOT_CLASS(Chunk, Node)
	private:
		std::atomic<int> amountNodes = 0, amountVertices = 0, amountFaces = 0;
		std::atomic<int> meshInstanceId = 0;
		std::atomic<bool> navigatable = false;
		std::atomic<bool> building = false;
		std::atomic<bool> volumeBuilt = false;
		
		Vector3 offset, cog;

		std::shared_ptr<Section> section;
		std::shared_ptr<VoxelData> volume;
		float** vertices;
		int** faces;
		unordered_map<size_t, std::shared_ptr<GraphNavNode>>* nodes;
		
		OpenSimplexNoise* noise;

		std::shared_mutex CHUNK_NODES_MUTEX;

		Voxel* intersection(int x, int y, int z);
	public:
		static void _register_methods();

		Chunk() : Chunk(Vector3(0, 0, 0)) {};
		Chunk(Vector3 offset);
		~Chunk();

		void _init(); // our initializer called by Godot
		
		Vector3 getOffset() {
			return offset;
		};
		Vector3 getCenterOfGravity() {
			return cog;
		};
		std::shared_ptr<Section> getSection() {
			return section;
		};
		std::shared_ptr<VoxelData> getVolume() {
			return volume;
		};
		int getMeshInstanceId() {
			return meshInstanceId;
		};
		bool isNavigatable() {
			return navigatable;
		};
		bool isBuilding() {
			return building;
		};
		float** getVertices() {
			return vertices;
		};
		int** getFaces() {
			return faces;
		};
		int getAmountVertices() {
			return amountVertices;
		};
		int getAmountFaces() {
			return amountFaces;
		};
		float isVoxel(int x, int y, int z);
		float getVoxel(int x, int y, int z);
		Voxel* getVoxelRay(Vector3 from, Vector3 to);
		std::shared_ptr<GraphNavNode> getNode(size_t hash);
		std::shared_ptr<GraphNavNode> findNode(Vector3 position);
		vector<std::shared_ptr<GraphNavNode>> getVoxelNodes(Vector3 voxelPosition);
		vector<std::shared_ptr<GraphNavNode>> getReachableNodes(std::shared_ptr<GraphNavNode> node);
		PoolVector3Array getReachableVoxels(Vector3 voxelPosition);
		PoolVector3Array getReachableVoxelsOfType(Vector3 voxelPosition, int type);
		void forEachNode(std::function<void(std::pair<size_t, std::shared_ptr<GraphNavNode>>)> func) {
			boost::shared_lock<std::shared_mutex> lock(CHUNK_NODES_MUTEX);
			std::for_each(nodes->begin(), nodes->end(), func);
		};

		void setOffset(Vector3 offset) {
			Chunk::offset = offset;
		};
		void setCenterOfGravity(Vector3 cog) {
			Chunk::cog = cog;
		};
		void setSection(std::shared_ptr<Section> section);
		void setNavigatable() {
			navigatable = true;
		};
		void setBuilding(bool building) {
			Chunk::building = building;
		};
		void setMeshInstanceId(int meshInstanceId) {
			Chunk::meshInstanceId = meshInstanceId;
		};
		void setVertices(float** vertices, int amountVertices) {
			if (Chunk::amountVertices > 0) {
				int i;
				for (i = 0; i < Chunk::amountVertices; i++)
					delete[] Chunk::vertices[i];
				delete[] Chunk::vertices;
			}

			Chunk::vertices = vertices;
			Chunk::amountVertices = amountVertices;
		};
		void setFaces(int** faces, int amountFaces) {
			if (Chunk::amountFaces > 0) {
				int i;
				for (i = 0; i < Chunk::amountFaces; i++)
					delete[] Chunk::faces[i];
				delete[] Chunk::faces;
			}

			Chunk::faces = faces;
			Chunk::amountFaces = amountFaces;
		};
		void setVoxel(int x, int y, int z, float v);
		void buildVolume();
		void addNode(std::shared_ptr<GraphNavNode> node);
		void removeNode(std::shared_ptr<GraphNavNode> node);
	};

}

#endif