#tool

extends MeshInstance
class_name QuadTreeMesh

var root: QuadTreeQuad
export(NodePath) var camera_path
onready var camera : Spatial = get_node(camera_path)
var quad_data : Dictionary
var mesh_data : Array

func _ready():
	rebuild_geom()

func _init():
	mesh = ArrayMesh.new()
	mesh_data = []
	quad_data = {
		"parent": null,
		"quad": null,
		"child_index": 0,
		"level": 2,
		"xorg": 0,
		"zorg": 0
	}
	root = QuadTreeQuad.new(quad_data)

#func _process(delta):
#	if !camera || !root: return
#	root.update(quad_data, to_local(camera.global_transform.origin))

func create_mesh():
	root.update(quad_data, to_local(camera.global_transform.origin))
	var vertices = []
	var indices = []
	var counts = [0, 0]
	vertices.resize(256)
	indices.resize(256)
	root.build_mesh(quad_data, vertices, indices, counts)
	#mesh_data = root.get_mesh_data()
	
	for data in mesh_data:
		mesh.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, data)

func rebuild_geom():
	if mesh && mesh.get_surface_count() > 0:
		for s in range(mesh.get_surface_count()):
			mesh.surface_remove(s)
	
	mesh_data.clear()
	create_mesh()
