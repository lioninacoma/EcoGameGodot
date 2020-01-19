#ifndef VOXEL_H
#define VOXEL_H

#include <Godot.hpp>
#include <Reference.hpp>
#include <Vector3.hpp>

namespace godot {

	class Voxel : public Reference {
		GODOT_CLASS(Voxel, Reference)

	private:
		Vector3 position;
		Vector3 chunkOffset;
		char type;
	public:
		static void _register_methods() {
			register_method("getPosition", &Voxel::getPosition);
			register_method("getType", &Voxel::getType);
		}

		Voxel() : Voxel(Vector3(), 0) {}
		Voxel(Vector3 position, char type) {
			Voxel::position = position;
			Voxel::type = type;
		}

		~Voxel() {

		}

		// our initializer called by Godot
		void _init() {

		}

		Vector3 getPosition() {
			return position;
		}

		char getType() {
			return type;
		}

		void setPosition(Vector3 position) {
			Voxel::position = position;
		}

		void setChunkOffset(Vector3 chunkOffset) {
			Voxel::chunkOffset = chunkOffset;
		}

		void setType(char type) {
			Voxel::type = type;
		}
	};
}

#endif