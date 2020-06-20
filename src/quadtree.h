// quadtree.h	-thatcher 9/15/1999 Copyright 1999-2000 Thatcher Ulrich
//
// Data structures for quadtree terrain storage.
//
// This code may be freely modified and redistributed.  I make no
// warrantees about it; use at your own risk.  If you do incorporate
// this code into a project, I'd appreciate a mention in the credits.
//
// Thatcher Ulrich <tu@tulrich.com>

#ifndef QUADTREE_H
#define QUADTREE_H

#include <Godot.hpp>
#include <Node.hpp>
#include <String.hpp>
#include <Vector3.hpp>

#include <shared_mutex>
#include <boost/thread/mutex.hpp>

#include <atomic>
#include <vector>

using namespace std;

#define QUADTREE_LEVEL 4

typedef unsigned short uint16;
typedef unsigned int uint32;
typedef short int16;
typedef int int32;

static float DetailThreshold = 40;

namespace godot {

	class quadsquare;

	// A structure used during recursive traversal of the tree to hold
	// relevant but transitory data.
	struct quadcornerdata {
		quadcornerdata* Parent;
		quadsquare* Square;
		int ChildIndex;
		int Level;
		int xorg, zorg;

		// root quadcorner data
		quadcornerdata() {
			Parent = NULL;
			Square = NULL;
			ChildIndex = 0;
			Level = QUADTREE_LEVEL;
			xorg = 0;
			zorg = 0;
		};
	};

	class quadsquare : public Node {
		GODOT_CLASS(quadsquare, Node)
	private:
		vector<quadsquare*> Child;
		Vector3 offset;

		unsigned char EnabledFlags; // bits 0-7: e, n, w, s, ne, nw, sw, se
		int SubEnabledCount[2]; // e, s enabled reference counts.
		std::atomic<bool> building = false;
		std::atomic<int> meshInstanceId = 0;

		std::shared_mutex CHILD_MUTEX;
		std::shared_mutex ENABLED_FLAGS_MUTEX;

		void EnableEdgeVertex(int index, bool IncrementCount, quadcornerdata* cd);
		quadsquare* EnableDescendant(int count, int stack[], quadcornerdata* cd);
		void EnableChild(int index, quadcornerdata* cd);
		void NotifyChildDisable(quadcornerdata* cd, int index);

		void setEnabledFlags(int pos, bool set);
		bool getEnabledFlags(int pos);
		bool isEmptyEnabledFlags();
		quadsquare* getChild(int index);
		void setChild(int index, quadsquare* square);
		void removeChild(int index);

		quadsquare* GetNeighbor(int dir, quadcornerdata* cd);
		void CreateChild(int index, quadcornerdata* cd);
		void SetupCornerData(quadcornerdata* q, quadcornerdata* cd, int ChildIndex);
	public:
		static void _register_methods();

		quadsquare() : quadsquare(Vector3(0, 0, 0)) {};
		quadsquare(Vector3 offset) : quadsquare(new quadcornerdata(), offset) {};
		quadsquare(quadcornerdata* pcd, Vector3 offset);
		~quadsquare();

		void _init(); // our initializer called by Godot

		void setBuilding(bool building);
		void setMeshInstanceId(int meshInstanceId);
		void update(quadcornerdata* cd, const float ViewerLocation[3]);
		void buildVertices(quadcornerdata* cd, float** vertices, int** faces, int* counts, int* minLevel);
		
		bool isBuilding();
		int getMeshInstanceId();
		Vector3 getOffset();
	};
}

#endif // QUADTREE_H
