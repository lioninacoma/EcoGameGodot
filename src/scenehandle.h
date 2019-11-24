#ifndef SCENEHANDLE_H
#define SCENEHANDLE_H

#include <Godot.hpp>
#include <SceneTree.hpp>
#include <ViewPort.hpp>
#include <Node.hpp>

using namespace std;

namespace godot {

	class SceneHandle : public Node {
	public:
		static SceneHandle* getInstance() {
			static auto handle = SceneHandle::_new();
			return (SceneHandle*)handle;
		}

		void _init() {

		}

		Node* getGame() {
			return get_tree()->get_root()->get_node("EcoGame");
		}
	};

}

#endif
