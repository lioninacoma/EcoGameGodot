#include "ecogame.h"

using namespace godot;

void EcoGame::_register_methods() {
	register_method("ready", &EcoGame::ready);
	register_method("buildChunk", &EcoGame::buildChunk);
}

EcoGame::EcoGame() {
	
}

EcoGame::~EcoGame() {
	// add your cleanup here
}

void EcoGame::_init() {
	// initialize any variables here
}

void EcoGame::ready() {
	SceneHandle* handle = SceneHandle::getInstance();
	Node* game = get_tree()->get_root()->get_node("EcoGame");
	game->add_child(handle);
}

void EcoGame::buildChunk(Variant vChunk) {
	Chunk* chunk = as<Chunk>(vChunk.operator Object * ());
	Node* game = get_tree()->get_root()->get_node("EcoGame");
	chunkBuilder.build(chunk, game);
}

