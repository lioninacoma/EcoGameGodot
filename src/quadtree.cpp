// quadtree.cpp	-thatcher 9/15/1999 Copyright 1999-2000 Thatcher Ulrich
//
// Code for quadtree terrain manipulation, meshing, and display.
//
// This code may be freely modified and redistributed.  I make no
// warrantees about it; use at your own risk.  If you do incorporate
// this code into a project, I'd appreciate a mention in the credits.
//
// Thatcher Ulrich <tu@tulrich.com>

#include "quadtree.h"

using namespace godot;

void quadsquare::_register_methods() {
	register_method("getMeshInstanceId", &quadsquare::getMeshInstanceId);
	register_method("setMeshInstanceId", &quadsquare::setMeshInstanceId);
}

//
// quadsquare functions.
//

quadsquare::quadsquare(quadcornerdata* pcd, Vector3 offset) : SubEnabledCount { 0, 0 }
// Constructor.
{
	pcd->Square = this;

	quadsquare::offset = offset;
	Child.resize(4);

	int i;
	for (i = 0; i < 4; i++) {
		Child[i] = NULL;
	}

	EnabledFlags = 0;
}

quadsquare::~quadsquare()
// Destructor.
{
	// Recursively delete sub-trees.
	int	i;
	for (i = 0; i < 4; i++)
		removeChild(i);
}

void quadsquare::_init() {

}

void quadsquare::setEnabledFlags(int pos, bool set) {
	boost::unique_lock<std::shared_mutex> lock(ENABLED_FLAGS_MUTEX);
	if (set) {
		EnabledFlags |= 1 << pos;
	}
	else {
		EnabledFlags &= ~(1 << pos);
	}
}

bool quadsquare::getEnabledFlags(int pos) {
	boost::shared_lock<std::shared_mutex> lock(ENABLED_FLAGS_MUTEX);
	return EnabledFlags & (1 << pos);
}

bool quadsquare::isEmptyEnabledFlags() {
	boost::shared_lock<std::shared_mutex> lock(ENABLED_FLAGS_MUTEX);
	return EnabledFlags == 0;
}

quadsquare* quadsquare::getChild(int index) {
	if (index < 0 || index > 3) return NULL;
	boost::shared_lock<std::shared_mutex> lock(CHILD_MUTEX);
	return Child[index];
}

void quadsquare::setChild(int index, quadsquare* child) {
	if (index < 0 || index > 3) return;
	boost::unique_lock<std::shared_mutex> lock(CHILD_MUTEX);
	Child[index] = child;
}

void quadsquare::removeChild(int index) {
	if (index < 0 || index > 3) return;
	boost::unique_lock<std::shared_mutex> lock(CHILD_MUTEX);
	if (!Child[index]) return;
	Child[index]->free();
	Child[index] = NULL;
}

void quadsquare::setMeshInstanceId(int meshInstanceId) {
	quadsquare::meshInstanceId = meshInstanceId;
}

void quadsquare::setBuilding(bool building) {
	quadsquare::building = building;
}

int quadsquare::getMeshInstanceId() {
	return meshInstanceId;
}

Vector3 quadsquare::getOffset() {
	return offset;
}

bool quadsquare::isBuilding() {
	return building;
}

quadsquare* quadsquare::GetNeighbor(int dir, quadcornerdata* cd)
// Traverses the tree in search of the quadsquare neighboring this square to the
// specified direction.  0-3 --> { E, N, W, S }.
// Returns NULL if the neighbor is outside the bounds of the tree.
{
	// If we don't have a parent, then we don't have a neighbor.
	// (Actually, we could have inter-tree connectivity at this level
	// for connecting separate trees together.)
	if (!cd->Parent) return NULL;

	// Find the parent and the child-index of the square we want to locate or create.
	quadsquare* p = NULL;

	int index = cd->ChildIndex ^ 1 ^ ((dir & 1) << 1);
	bool SameParent = ((dir - cd->ChildIndex) & 2) ? true : false;

	if (SameParent) {
		p = cd->Parent->Square;
	}
	else {
		p = cd->Parent->Square->GetNeighbor(dir, cd->Parent);
		if (!p) return NULL;
	}

	return p->getChild(index);
}

void quadsquare::EnableEdgeVertex(int index, bool IncrementCount, quadcornerdata* cd)
// Enable the specified edge vertex.  Indices go { e, n, w, s }.
// Increments the appropriate reference-count if IncrementCount is true.
{
	if (getEnabledFlags(index) && !IncrementCount) return;

	// Turn on flag and deal with reference count.
	
	setEnabledFlags(index, true);
	if (IncrementCount && (index == 0 || index == 3)) {
		SubEnabledCount[index & 1]++;
	}

	// Now we need to enable the opposite edge vertex of the adjacent square (i.e. the alias vertex).

	// This is a little tricky, since the desired neighbor node may not exist, in which
	// case we have to create it, in order to prevent cracks.  Creating it may in turn cause
	// further edge vertices to be enabled, propagating updates through the tree.

	// The sticking point is the quadcornerdata list, which
	// conceptually is just a linked list of activation structures.
	// In this function, however, we will introduce branching into
	// the "list", making it in actuality a tree.  This is all kind
	// of obscure and hard to explain in words, but basically what
	// it means is that our implementation has to be properly
	// recursive.

	// Travel upwards through the tree, looking for the parent in common with our desired neighbor.
	// Remember the path through the tree, so we can travel down the complementary path to get to the neighbor.
	quadsquare* p = this;
	quadcornerdata* pcd = cd;
	int ct = 0;
	int stack[32];
	for (;;) {
		int ci = pcd->ChildIndex;

		if (!pcd->Parent || !pcd->Parent->Square) {
			// Neighbor doesn't exist (it's outside the tree), so there's no alias vertex to enable.
			return;
		}
		p = pcd->Parent->Square;
		pcd = pcd->Parent;

		bool SameParent = ((index - ci) & 2) ? true : false;

		ci = ci ^ 1 ^ ((index & 1) << 1); // Child index of neighbor node.

		stack[ct] = ci;
		ct++;

		if (SameParent) break;
	}

	// Get a pointer to our neighbor (create if necessary), by walking down
	// the quadtree from our shared ancestor.
	p = p->EnableDescendant(ct, stack, pcd);

	// Finally: enable the vertex on the opposite edge of our neighbor, the alias of the original vertex.
	index ^= 2;
	p->setEnabledFlags(index, true);
	if (IncrementCount && (index == 0 || index == 3)) {
		p->SubEnabledCount[index & 1]++;
	}
}

quadsquare* quadsquare::EnableDescendant(int count, int path[], quadcornerdata* cd)
// This function enables the descendant node 'count' generations below
// us, located by following the list of child indices in path[].
// Creates the node if necessary, and returns a pointer to it.
{
	count--;
	int ChildIndex = path[count];

	if (!getEnabledFlags(ChildIndex + 4)) {
		EnableChild(ChildIndex, cd);
	}

	auto child = getChild(ChildIndex);

	if (count > 0) {
		quadcornerdata q;
		SetupCornerData(&q, cd, ChildIndex);
		return child->EnableDescendant(count, path, &q);
	}
	else {
		return child;
	}
}

void quadsquare::CreateChild(int index, quadcornerdata* cd)
// Creates a child square at the specified index.
{
	if (!getChild(index)) {
		quadcornerdata q;
		SetupCornerData(&q, cd, index);

		auto child = quadsquare::_new();
		child->offset = offset;
		q.Square = child;
		setChild(index, child);
	}
}

void quadsquare::EnableChild(int index, quadcornerdata* cd)
// Enable the indexed child node.  { ne, nw, sw, se }
// Causes dependent edge vertices to be enabled.
{
	if (!getEnabledFlags(index + 4)) {
		setEnabledFlags(index + 4, true);
		EnableEdgeVertex(index, true, cd);
		EnableEdgeVertex((index + 1) & 3, true, cd);

		if (!getChild(index)) {
			CreateChild(index, cd);
		}
	}
}

void quadsquare::NotifyChildDisable(quadcornerdata* cd, int index)
// Marks the indexed child quadrant as disabled.  Deletes the child node
// if it isn't static.
{
	// Clear enabled flag for the child.
	setEnabledFlags(index + 4, false);

	// Update child enabled counts for the affected edge verts.
	quadsquare* s;

	// e: neighbour[n]->SubEnabledCount[s]-- &&         this->SubEnabledCount[e]--
	// n: neighbour[n]->SubEnabledCount[s]-- && neighbour[w]->SubEnabledCount[e]--
	// w:         this->SubEnabledCount[s]-- && neighbour[w]->SubEnabledCount[e]--
	// s:         this->SubEnabledCount[s]-- &&	        this->SubEnabledCount[e]--

	if (index & 2) // w or s
		s = this;
	else s = GetNeighbor(1, cd); // n
	if (s) {
		//s->SubEnabledCount[1] = max(s->SubEnabledCount[1] - 1, 0); // s
		s->SubEnabledCount[1]--;
	}
	
	if (index == 1 || index == 2) // n or w
		s = GetNeighbor(2, cd); // w
	else s = this;
	if (s) {
		//s->SubEnabledCount[0] = max(s->SubEnabledCount[0] - 1, 0); // e
		s->SubEnabledCount[0]--;
	}

	removeChild(index);
}

float VertexDist(float x, float z, const float Viewer[3])
{
	float dx = fabs(x - Viewer[0]);
	float dz = fabs(z - Viewer[2]);
	float d = dx;
	if (dz > d) d = dz;

	return d;
}

bool VertexTest(float x, float z, const float Viewer[3])
// Returns true if the vertex at (x,z) with the given world-space error between
// its interpolated location and its true location, should be enabled, given that
// the viewpoint is located at Viewer[].
{
	return DetailThreshold > VertexDist(x, z, Viewer);
}

float BoxDist(float x, float z, float size, const float Viewer[3])
{
	// Find the minimum distance to the box.
	float half = size * 0.5;
	float dx = fabs(x + half - Viewer[0]) - half;
	float dz = fabs(z + half - Viewer[2]) - half;
	float d = dx;
	if (dz > d) d = dz;

	return d;
}

bool BoxTest(float x, float z, float size, const float Viewer[3])
// Returns true if any vertex within the specified box (origin at x,z,
// edges of length size) with the given error value could be enabled
// based on the given viewer location.
{
	return DetailThreshold > BoxDist(x, z, size, Viewer);
}

static const int DIST_FACTOR = 2.0;

void quadsquare::update(quadcornerdata* cd, const float ViewerLocation[3])
// Does the actual work of updating enabled states and tree growing/shrinking.
{
	int half = 1 << cd->Level;
	int whole = half << 1;

	/*float d = BoxDist(cd->xorg, cd->zorg, whole, ViewerLocation);
	float max_d = DetailThreshold;
	int min_level = QUADTREE_LEVEL - 1;

	if (d <= max_d) {
		min_level = ceil(pow(pow(QUADTREE_LEVEL, DIST_FACTOR) * d / max_d, 1.0 / DIST_FACTOR));
	}*/

	// See about enabling child verts.
	if (!getEnabledFlags(0) && VertexTest(cd->xorg + whole, cd->zorg + half, ViewerLocation))
		EnableEdgeVertex(0, false, cd); // East vert.
	if (!getEnabledFlags(3) && VertexTest(cd->xorg + half, cd->zorg + whole, ViewerLocation))
		EnableEdgeVertex(3, false, cd); // South vert.
	if (cd->Level > 0) {
		if (!getEnabledFlags(5)) {
			if (BoxTest(cd->xorg, cd->zorg, half, ViewerLocation))
				EnableChild(1, cd); // nw child.er
		}
		if (!getEnabledFlags(4)) {
			if (BoxTest(cd->xorg + half, cd->zorg, half, ViewerLocation))
				EnableChild(0, cd); // ne child.
		}
		if (!getEnabledFlags(6)) {
			if (BoxTest(cd->xorg, cd->zorg + half, half, ViewerLocation))
				EnableChild(2, cd); // sw child.
		}
		if (!getEnabledFlags(7)) {
			if (BoxTest(cd->xorg + half, cd->zorg + half, half, ViewerLocation))
				EnableChild(3, cd); // se child.
		}

		// Recurse into child quadrants as necessary.
		quadsquare* child;
		quadcornerdata q;

		if (getEnabledFlags(5)) {
			SetupCornerData(&q, cd, 1);
			child = getChild(1);
			if (child) child->update(&q, ViewerLocation);
		}
		if (getEnabledFlags(4)) {
			SetupCornerData(&q, cd, 0);
			child = getChild(0);
			if (child) child->update(&q, ViewerLocation);
		}
		if (getEnabledFlags(6)) {
			SetupCornerData(&q, cd, 2);
			child = getChild(2);
			if (child) child->update(&q, ViewerLocation);
		}
		if (getEnabledFlags(7)) {
			SetupCornerData(&q, cd, 3);
			child = getChild(3);
			if (child) child->update(&q, ViewerLocation);
		}
	}

	// Test for disabling east, south, and center.
	if (getEnabledFlags(0) && SubEnabledCount[0] == 0 && !VertexTest(cd->xorg + whole, cd->zorg + half, ViewerLocation)) {
		setEnabledFlags(0, false); // e
		quadsquare* s = GetNeighbor(0, cd);
		if (s) s->setEnabledFlags(2, false); // w
	}
	if (getEnabledFlags(3) && SubEnabledCount[1] == 0 && !VertexTest(cd->xorg + half, cd->zorg + whole, ViewerLocation)) {
		setEnabledFlags(3, false); // s
		quadsquare* s = GetNeighbor(3, cd);
		if (s) s->setEnabledFlags(1, false); // n
	}

	// Test for disabling edge cases x = 0, z = 0
	if (EnabledFlags == 4 && cd->xorg == 0 && SubEnabledCount[0] == 0 && !VertexTest(cd->xorg, cd->zorg + half, ViewerLocation)) {
		setEnabledFlags(2, false); // w
	}
	if (EnabledFlags == 2 && cd->zorg == 0 && SubEnabledCount[1] == 0 && !VertexTest(cd->xorg + half, cd->zorg, ViewerLocation)) {
		setEnabledFlags(1, false); // n
	}
	if (EnabledFlags == 6 && cd->xorg == 0 && cd->zorg == 0 && SubEnabledCount[0] == 0 && SubEnabledCount[1] == 0 && !VertexTest(cd->xorg, cd->zorg, ViewerLocation)) {
		setEnabledFlags(2, false); // w
		setEnabledFlags(1, false); // n
	}

	// Disable ourself.
	if (isEmptyEnabledFlags() && cd->Parent != NULL && !BoxTest(cd->xorg, cd->zorg, whole, ViewerLocation)) {
		cd->Parent->Square->NotifyChildDisable(cd->Parent, cd->ChildIndex); // nb: possibly deletes 'this'.
	}
}

static void tri(int index, int offset, int a, int b, int c, int** faces)
{
	faces[index][0] = offset + a;
	faces[index][1] = offset + b;
	faces[index][2] = offset + c;
}

static void initVert(int index, float x, float y, float z, float** vertices)
// Initializes the indexed vertex of VertexArray[] with the
// given values.
{
	vertices[index][0] = x;
	vertices[index][1] = y;
	vertices[index][2] = z;
}

void quadsquare::buildVertices(quadcornerdata* cd, float** vertices, int** faces, int* counts, int* minLevel)
// Does the work of rendering this square.  Uses the enabled vertices only.
// Recurses as necessary.
{
	int half = 1 << cd->Level;
	int whole = 2 << cd->Level;

	minLevel[0] = min(minLevel[0], cd->Level);

	int i;

	int flags = 0;
	int mask = 1;
	quadsquare* child;
	quadcornerdata q;
	for (i = 0; i < 4; i++, mask <<= 1) {
		//if (EnabledFlags & (16 << i)) {
		if (getEnabledFlags(i + 4)) {
			SetupCornerData(&q, cd, i);
			child = getChild(i);
			if (child) child->buildVertices(&q, vertices, faces, counts, minLevel);
		}
		else {
			flags |= mask;
		}
	}

	if (flags == 0) return;

	int indexOffset = counts[0];

	// Init vertex data.
	initVert(counts[0]++, cd->xorg + half, 0, cd->zorg + half, vertices);
	initVert(counts[0]++, cd->xorg + whole, 0, cd->zorg + half, vertices);
	initVert(counts[0]++, cd->xorg + whole, 0, cd->zorg, vertices);
	initVert(counts[0]++, cd->xorg + half, 0, cd->zorg, vertices);
	initVert(counts[0]++, cd->xorg, 0, cd->zorg, vertices);
	initVert(counts[0]++, cd->xorg, 0, cd->zorg + half, vertices);
	initVert(counts[0]++, cd->xorg, 0, cd->zorg + whole, vertices);
	initVert(counts[0]++, cd->xorg + half, 0, cd->zorg + whole, vertices);
	initVert(counts[0]++, cd->xorg + whole, 0, cd->zorg + whole, vertices);

	// Make the list of triangles to draw.
	//if ((EnabledFlags & 1) == 0)
	if (!getEnabledFlags(0)) tri(counts[1]++, indexOffset, 0, 8, 2, faces);
	else {
		if (flags & 8) tri(counts[1]++, indexOffset, 0, 8, 1, faces);
		if (flags & 1) tri(counts[1]++, indexOffset, 0, 1, 2, faces);
	}
	//if ((EnabledFlags & 2) == 0)
	if (!getEnabledFlags(1)) tri(counts[1]++, indexOffset, 0, 2, 4, faces);
	else {
		if (flags & 1) tri(counts[1]++, indexOffset, 0, 2, 3, faces);
		if (flags & 2) tri(counts[1]++, indexOffset, 0, 3, 4, faces);
	}
	//if ((EnabledFlags & 4) == 0)
	if (!getEnabledFlags(2)) tri(counts[1]++, indexOffset, 0, 4, 6, faces);
	else {
		if (flags & 2) tri(counts[1]++, indexOffset, 0, 4, 5, faces);
		if (flags & 4) tri(counts[1]++, indexOffset, 0, 5, 6, faces);
	}
	//if ((EnabledFlags & 8) == 0)
	if (!getEnabledFlags(3)) tri(counts[1]++, indexOffset, 0, 6, 8, faces);
	else {
		if (flags & 4) tri(counts[1]++, indexOffset, 0, 6, 7, faces);
		if (flags & 8) tri(counts[1]++, indexOffset, 0, 7, 8, faces);
	}
}

void quadsquare::SetupCornerData(quadcornerdata* q, quadcornerdata* cd, int ChildIndex)
// Fills the given structure with the appropriate corner values for the
// specified child block, given our own vertex data and our corner
// vertex data from cd->
//
// ChildIndex mapping:
// +-+-+
// |1|0|
// +-+-+
// |2|3|
// +-+-+
//
// Verts mapping:
// 1-0
// | |
// 2-3
//
// Vertex mapping:
// +-2-+
// | | |
// 3-0-1
// | | |
// +-4-+
{
	int half = 1 << cd->Level;

	q->Parent = cd;
	q->Square = getChild(ChildIndex);
	q->Level = cd->Level - 1;
	q->ChildIndex = ChildIndex;

	switch (ChildIndex) {
	default:
	case 0:
		q->xorg = cd->xorg + half;
		q->zorg = cd->zorg;
		break;

	case 1:
		q->xorg = cd->xorg;
		q->zorg = cd->zorg;
		break;

	case 2:
		q->xorg = cd->xorg;
		q->zorg = cd->zorg + half;
		break;

	case 3:
		q->xorg = cd->xorg + half;
		q->zorg = cd->zorg + half;
		break;
	}
}
