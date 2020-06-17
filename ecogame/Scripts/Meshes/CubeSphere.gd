tool

extends MeshInstance
class_name CubeSphere

export(int) var radius = 1 setget set_radius

var arr = []
var verts : PoolVector3Array
var normals : PoolVector3Array
var indices : PoolIntArray

var quads = []

func _ready():
	arr = []
	verts = PoolVector3Array()
	normals = PoolVector3Array()
	indices = PoolIntArray()
	rebuild_geom()

func set_radius(val):
	radius = val
	rebuild_geom()

func set_vertex(vert : Vector3):
#	var v = Vector3(vert.x, vert.y, vert.z) * 2.0 / radius - Vector3.ONE
#	var x2 = v.x * v.x
#	var y2 = v.y * v.y
#	var z2 = v.z * v.z
#	var s = Vector3()
#	s.x = v.x * sqrt(1.0 - y2 / 2.0 - z2 / 2.0 + y2 * z2 / 3.0)
#	s.y = v.y * sqrt(1.0 - x2 / 2.0 - z2 / 2.0 + x2 * z2 / 3.0)
#	s.z = v.z * sqrt(1.0 - x2 / 2.0 - y2 / 2.0 + x2 * y2 / 3.0)
#	verts.push_back(s * radius)
	var s = vert
	verts.push_back(s)
	normals.push_back(s)

func set_quad(v00 : int, v10 : int, v01 : int, v11 : int):
	indices.push_back(v00)
	indices.push_back(v10)
	indices.push_back(v01)
	indices.push_back(v10)
	indices.push_back(v11)
	indices.push_back(v01)

func create_triangles():
	var ring = (radius + radius) * 2;
	var v = 0
	for y in range(radius):
		for q in range(ring - 1):
			set_quad(v, v + 1, v + ring, v + ring + 1)
			v += 1
		set_quad(v, v - ring + 1, v + ring, v + 1)
		v += 1
	
	create_top_face(ring)
	create_bottom_face(ring)

func create_top_face(ring : int):
	var v = ring * radius;
	for x in range(radius - 1):
		set_quad(v, v + 1, v + ring - 1, v + ring)
		v += 1
	set_quad(v, v + 1, v + ring - 1, v + 2)
	
	var vMin = ring * (radius + 1) - 1
	var vMid = vMin + 1
	var vMax = v + 2
	
	for z in range(1, radius - 1):
		set_quad(vMin, vMid, vMin - 1, vMid + radius - 1)
		for x in range(1, radius - 1):
			set_quad(vMid, vMid + 1, vMid + radius - 1, vMid + radius)
			vMid += 1
		set_quad(vMid, vMax, vMid + radius - 1, vMax + 1)
		vMin -= 1
		vMid += 1
		vMax += 1
	
	var vTop = vMin - 2
	set_quad(vMin, vMid, vTop + 1, vTop)
	for x in range(1, radius - 1):
		set_quad(vMid, vMid + 1, vTop, vTop - 1)
		vTop -= 1
		vMid += 1
	set_quad(vMid, vTop - 2, vTop, vTop - 1)

func create_bottom_face(ring : int):
	var v = 1
	var vMid = verts.size() - (radius - 1) * (radius - 1)
	set_quad(ring - 1, vMid, 0, 1)
	for x in range(1, radius - 1):
		set_quad(vMid, vMid + 1, v, v + 1)
		v += 1
		vMid += 1
	set_quad(vMid, v + 2, v, v + 1)

	var vMin = ring - 2
	vMid -= radius - 2
	var vMax = v + 2

	for z in range(1, radius - 1):
		set_quad(vMin, vMid + radius - 1, vMin + 1, vMid)
		for x in range(1, radius - 1):
			set_quad(vMid + radius - 1, vMid + radius, vMid, vMid + 1)
			vMid += 1
		set_quad(vMid + radius - 1, vMax + 1, vMid, vMax)
		vMin -= 1
		vMid += 1
		vMax += 1

	var vTop = vMin - 1
	set_quad(vTop + 1, vTop, vTop + 2, vMid)
	for x in range(1, radius - 1):
		set_quad(vTop, vTop - 1, vMid, vMid + 1)
		vTop -= 1
		vMid += 1
	set_quad(vTop, vTop - 1, vMid, vTop - 2)

func create_mesh():
	if !mesh: return
	
	arr.resize(Mesh.ARRAY_MAX)
	
	for y in range(radius + 1):
		for x in range(radius + 1):
			set_vertex(Vector3(x, y, 0))
		for z in range(1, radius + 1):
			set_vertex(Vector3(radius, y, z))
		for x in range(radius - 1, -1, -1):
			set_vertex(Vector3(x, y, radius))
		for z in range(radius - 1, 0, -1):
			set_vertex(Vector3(0, y, z))
	for z in range(1, radius):
		for x in range(1, radius):
			set_vertex(Vector3(x, radius, z))
	for z in range(1, radius):
		for x in range(1, radius):
			set_vertex(Vector3(x, 0, z))
	
#	for i in range (2):
#		verts.push_back(Vector3(randi() % radius, randi() % radius, randi() % radius))
	
#	for i in range (100):
#		set_vertex(Vector3(randi() % radius, randi() % radius, randi() % radius))
	
	create_triangles()
	
	arr[Mesh.ARRAY_VERTEX] = verts
	arr[Mesh.ARRAY_NORMAL] = normals
	arr[Mesh.ARRAY_INDEX] = indices
	
	mesh.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, arr)
	mesh.surface_set_name(0, "surface0")

func rebuild_geom():
	if mesh && mesh.surface_find_by_name("surface0") >= 0:
		mesh.surface_remove(0)
	
	arr.clear()
	verts.resize(0)
	normals.resize(0)
	indices.resize(0)
	
	create_mesh()
