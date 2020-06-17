tool

extends MeshInstance
class_name QuadTreeMesh

var root: QuadTreeQuad
export(NodePath) var camera_path
onready var camera : Spatial = get_node(camera_path)
var quad_data : Dictionary
var camera_pos_before = null

func _init():
	mesh = ArrayMesh.new()
	quad_data = {
		"parent": null,
		"quad": null,
		"child_index": 0,
		"level": 4,
		"xorg": 0,
		"zorg": 0
	}
	root = QuadTreeQuad.new(quad_data)

func _process(delta):
	if mesh && mesh.get_surface_count() == 0:
		print ("rebuild")
		rebuild_geom()
		return
	
	var camera_pos = camera.global_transform.origin
	if camera_pos_before == null || camera_pos_before != camera_pos:
		camera_pos_before = camera_pos
		rebuild_geom()

func create_mesh():
	var arrays = []
	var vertex_array = PoolVector3Array()
	var normal_array = PoolVector3Array()
	var index_array = PoolIntArray()
	
	var vertices = []
	var indices = []
	var counts = [0, 0]
	
	vertices.resize(8192)
	indices.resize(8192)
	
	root.update(quad_data, to_local(camera.global_transform.origin))
	root.build_mesh(quad_data, vertices, indices, counts)
	
	vertex_array.resize(counts[0])
	normal_array.resize(counts[0])
	index_array.resize(counts[1])
	
	for v in range(counts[0]):
		vertex_array[v] = vertices[v]
		normal_array[v] = Vector3(0, 1, 0)
	for i in range(counts[1]):
		index_array[i] = indices[i]
	
	arrays.resize(Mesh.ARRAY_MAX)
	arrays[Mesh.ARRAY_VERTEX] = vertex_array
	arrays[Mesh.ARRAY_NORMAL] = normal_array
	arrays[Mesh.ARRAY_INDEX] = index_array
	
	mesh.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, arrays)

func add_normal(src, normal) -> Vector3:
	return (src + normal).normalized()

func rebuild_geom():
	if mesh && mesh.get_surface_count() > 0:
		for s in range(mesh.get_surface_count()):
			mesh.surface_remove(s)
	camera = get_node(camera_path)
	create_mesh()

#	normal_array.resize(amountVertices)
#	collision_array.resize(amountIndices)
	
#	for i in range(0, amountVertices, 3):
#		vertex_array[i] = Vector3(vertices[i], vertices[i + 1], vertices[i + 2])
#		normal_array[i] = Vector3(0, 0, 0)
#
#	for i in range(0, amountIndices, 3):
#		index_array[i] = indices[i];
#		index_array[i + 1] = indices[i + 1];
#		index_array[i + 2] = indices[i + 2];
#
#		var x0 : Vector3 = vertex_array[indices[i]]
#		var x1 : Vector3 = vertex_array[indices[i + 1]]
#		var x2 : Vector3 = vertex_array[indices[i + 2]]
#		var v0 : Vector3 = x0 - x2
#		var v1 : Vector3 = x1 - x2
#		var normal : Vector3 = v0.cross(v1).normalized()
#
#		normal_array[indices[i]] = add_normal(normal_array[indices[i]], normal)
#		normal_array[indices[i + 1]] = add_normal(normal_array[indices[i + 1]], normal)
#		normal_array[indices[i + 2]] = add_normal(normal_array[indices[i + 2]], normal)
#
#		collision_array[i] = vertex_array[indices[i]]
#		collision_array[i + 1] = vertex_array[indices[i + 1]]
#		collision_array[i + 2] = vertex_array[indices[i + 2]]
