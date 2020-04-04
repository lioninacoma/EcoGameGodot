#include "ecogame.h"
#include "voxel.h"
#include "chunk.h"
#include "graphnode.h"
#include "voxelworld.h"

extern "C" void GDN_EXPORT godot_gdnative_init(godot_gdnative_init_options *o) {
	godot::Godot::gdnative_init(o);
}

extern "C" void GDN_EXPORT godot_gdnative_terminate(godot_gdnative_terminate_options *o) {
	godot::Godot::gdnative_terminate(o);
}

extern "C" void GDN_EXPORT godot_nativescript_init(void *handle) {
	godot::Godot::nativescript_init(handle);

	godot::register_class<godot::EcoGame>();
	godot::register_class<godot::Voxel>();
	godot::register_class<godot::Chunk>();
	godot::register_class<godot::GraphNavNode>();
	godot::register_class<godot::VoxelWorld>();
}
