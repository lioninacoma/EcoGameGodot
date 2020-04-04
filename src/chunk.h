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

using namespace std;

namespace godot {

	class GraphNavNode;
	class GraphEdge;
	class VoxelWorld;
	class EcoGame;

	class Chunk : public Node {
		GODOT_CLASS(Chunk, Node)
	private:
		std::atomic<int> amountNodes = 0, amountVertices = 0, amountFaces = 0;
		std::atomic<int> meshInstanceId = 0;
		std::atomic<bool> navigatable = false;
		std::atomic<bool> building = false;
		std::atomic<bool> volumeBuilt = false;
		
		Vector3 offset, cog;

		std::shared_ptr<VoxelWorld> world;
		std::shared_ptr<VoxelData> volume;
		float** vertices;
		int** faces;
		unordered_map<size_t, std::shared_ptr<GraphNavNode>>* nodes;
		
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
		
		Vector3 getOffset() {
			return offset;
		};
		Vector3 getCenterOfGravity() {
			return cog;
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
		void forEachNode(std::function<void(std::pair<size_t, std::shared_ptr<GraphNavNode>>)> func) {
			boost::shared_lock<std::shared_mutex> lock(CHUNK_NODES_MUTEX);
			std::for_each(nodes->begin(), nodes->end(), func);
		};
		void forEachFace(std::function<void(int* face)> func) {
			boost::shared_lock<std::shared_mutex> lock(FACES_MUTEX);
			std::for_each(faces, faces + amountFaces, func);
		};
		void forEachVertex(std::function<void(float* vertex)> func) {
			boost::shared_lock<std::shared_mutex> lock(VERTICES_MUTEX);
			std::for_each(vertices, vertices + amountVertices, func);
		};

		void setWorld(std::shared_ptr<VoxelWorld> world) {
			Chunk::world = world;
		};
		void setOffset(Vector3 offset) {
			Chunk::offset = offset;
		};
		void setCenterOfGravity(Vector3 cog) {
			Chunk::cog = cog;
		};
		void setNavigatable() {
			navigatable = true;
		};
		void setBuilding(bool building) {
			Chunk::building = building;
		};
		void setMeshInstanceId(int meshInstanceId) {
			Chunk::meshInstanceId = meshInstanceId;
		};
		float* getVertex(int i) {
			if (i < 0 || i >= amountVertices) return NULL;
			boost::shared_lock<std::shared_mutex> lock(VERTICES_MUTEX);
			return vertices[i];
		};
		int* getFace(int i) {
			if (i < 0 || i >= amountFaces) return NULL;
			boost::shared_lock<std::shared_mutex> lock(FACES_MUTEX);
			return faces[i];
		};
		void setVertices(float** vertices, int amountVertices) {
			boost::unique_lock<std::shared_mutex> lock(VERTICES_MUTEX);
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
			boost::unique_lock<std::shared_mutex> lock(FACES_MUTEX);
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
		void addFaceNodes(Vector3 a, Vector3 b, Vector3 c);
		void addEdge(std::shared_ptr<GraphNavNode> a, std::shared_ptr<GraphNavNode> b, float cost);
		std::shared_ptr<GraphNavNode> fetchOrCreateNode(Vector3 position);
	};

}

#endif