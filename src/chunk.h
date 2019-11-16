#ifndef CHUNK_H
#define CHUNK_H

#include <Godot.hpp>
#include <Reference.hpp>
#include <MeshInstance.hpp>
#include <OpenSimplexNoise.hpp>

#include <vector>

#include <boost/function.hpp>
#include <boost/bind.hpp>

#include "constants.h"
#include "intersection.h"
#include "voxel.h"

using namespace std;

namespace godot {

	class Chunk : public Reference {
		GODOT_CLASS(Chunk, Reference)

	private:
		OpenSimplexNoise* noise;
		Vector3 offset;
		int meshInstanceId = 0;
		char* volume;

		int getVoxelNoiseY(int x, int z);
		float getVoxelNoiseChance(int x, int y, int z);
		int flattenIndex(int x, int y, int z);
		Voxel* intersection(int x, int y, int z);
	public:
		static void _register_methods();

		Chunk() : Chunk(Vector3(0, 0, 0)) {};
		Chunk(Vector3 offset);
		~Chunk();

		void _init(); // our initializer called by Godot
		
		// getter
		Vector3 getOffset() {
			return offset;
		}
		char* getVolume() {
			return volume;
		}
		int getMeshInstanceId() {
			return meshInstanceId;
		};
		int getVoxel(int x, int y, int z);
		Ref<Voxel> getVoxelRay(Vector3 from, Vector3 to);

		// setter
		void setOffset(Vector3 offset) {
			Chunk::offset = offset;
		};
		void setMeshInstanceId(int meshInstanceId) {
			Chunk::meshInstanceId = meshInstanceId;
		};
		void setVoxel(int x, int y, int z, int v);
		int buildVolume();
	};

}

#endif