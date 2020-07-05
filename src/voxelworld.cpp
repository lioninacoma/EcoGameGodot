#include "voxelworld.h"

using namespace godot;

void VoxelWorld::_register_methods() {
	register_method("_notification", &VoxelWorld::_notification);
}

VoxelWorld::VoxelWorld() {

}

VoxelWorld::~VoxelWorld() {

}

void VoxelWorld::_init() {

}

void VoxelWorld::_notification(const int64_t what) {
	if (what == Node::NOTIFICATION_READY) {}
}