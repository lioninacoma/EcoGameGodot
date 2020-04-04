#include "ecogame.h"
#include "navigator.h"
#include "threadpool.h"
#include "graphnode.h"

#include <boost/exception/diagnostic_information.hpp>
#include <boost/exception_ptr.hpp>

using namespace godot;

static std::shared_ptr<EcoGame> game;

void EcoGame::_register_methods() {
}

EcoGame::EcoGame() {
}

EcoGame::~EcoGame() {
}

std::shared_ptr<EcoGame> EcoGame::get() {
	return game;
};

void EcoGame::_init() {
	// initialize any variables here
	game = std::shared_ptr<EcoGame>(this);
}