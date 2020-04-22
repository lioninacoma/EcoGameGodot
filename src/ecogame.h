#ifndef ECOGAME_H
#define ECOGAME_H

#include <Godot.hpp>
#include <SceneTree.hpp>
#include <ViewPort.hpp>
#include <Node.hpp>
#include <String.hpp>
#include <Array.hpp>
#include <Vector3.hpp>

using namespace std;

namespace godot {
	class GraphNode;

	class EcoGame : public Node {
		GODOT_CLASS(EcoGame, Node)

	private:
	public:
		static std::shared_ptr<EcoGame> get();

		Node* getNode() {
			return get_tree()->get_root()->get_node("EcoGame");
		}

		static void _register_methods();

		EcoGame();
		~EcoGame();

		void _init();
	};

}

#endif