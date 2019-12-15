#ifndef FN_H
#define FN_H

#include <vector>

#include "constants.h"

namespace godot {
	namespace fn {
		template <class T>
		static bool contains(const std::vector<T>& vec, const T& value) {
			return std::find(vec.begin(), vec.end(), value) != vec.end();
		}

		static int fi3(int x, int y, int z, int w, int h) {
			return x + w * (y + h * z);
		}

		static int fi3(int x, int y, int z) {
			return fi3(x, y, z, CHUNK_SIZE_X, CHUNK_SIZE_Y);
		}

		static int fi2(int x, int z, int w) {
			return x + z * w;
		}

		static int fi2(int x, int z) {
			return fi2(x, z, CHUNK_SIZE_X);
		}

		static Vector3 toChunkCoords(Vector3 position) {
			int ix = (int)position.x;
			int iy = (int)position.y;
			int iz = (int)position.z;
			int chunkX = ix / CHUNK_SIZE_X;
			int chunkY = iy / CHUNK_SIZE_Y;
			int chunkZ = iz / CHUNK_SIZE_Z;
			position.x = chunkX;
			position.y = chunkY;
			position.z = chunkZ;
			return position;
		}

		static Vector2 toChunkCoords(Vector2 position) {
			Vector3 chunkCoords = toChunkCoords(Vector3(position.x, 0, position.y));
			return Vector2(chunkCoords.x, chunkCoords.z);
		}

		static Vector3 toSectionCoords(Vector3 position) {
			int ix = (int)position.x;
			int iz = (int)position.z;
			int sectionX = ix / (SECTION_SIZE * CHUNK_SIZE_X);
			int sectionZ = iz / (SECTION_SIZE * CHUNK_SIZE_Z);
			position.x = sectionX;
			position.y = 0;
			position.z = sectionZ;
			return position;
		}

		static Vector2 toSectionCoords(Vector2 position) {
			Vector3 sectionCoords = toSectionCoords(Vector3(position.x, 0, position.y));
			return Vector2(sectionCoords.x, sectionCoords.z);
		}
	}
}

#endif