extends MeshInstance
class_name QuadTreeWorld

var VoxelWorldNative = load("res://bin/VoxelWorld.gdns")

# config
var voxel_body
var material : SpatialMaterial
export(NodePath) var camera_path
onready var camera : Spatial = get_node(camera_path)
var camera_pos_before = null
const TIME_PERIOD = 0.05 # 50ms
var time = 0

func _init():
	self.material = WorldVariables.stoneMaterial
	if !mesh: mesh = ArrayMesh.new()
	voxel_body = VoxelWorldNative.new()
	add_child(voxel_body)
#	voxel_body.buildQuadTrees(Vector3(-10, 0, -10))

func _process(delta):
	if !camera_path: return
	
	if !camera:
		camera = get_node(camera_path)
		return
	
	var camera_pos = camera.global_transform.origin
	
	time += delta
	if time > TIME_PERIOD && (camera_pos_before == null 
		|| camera_pos_before != camera_pos):
		camera_pos_before = camera_pos
		voxel_body.buildQuadTrees(camera_pos)
		time = 0

func delete_quadtree(quadtree, world) -> void:
	for i in range(mesh.get_surface_count()):
		mesh.surface_remove(i)
#	var old_mesh_instance_id = quadtree.getMeshInstanceId()
#	if old_mesh_instance_id != 0:
#		mesh.surface_remove(old_mesh_instance_id - 1)

func build_quadtree(mesh_data : Array, quadtree, world) -> void:
	if !mesh_data || !quadtree: return
	delete_quadtree(quadtree, world)
	var id = build_mesh(mesh_data, world)
	
#	var a = mesh.surface_get_arrays(0)
#	var m = mesh.surface_get_material(0)
#	mesh.surface_remove(0)
#	mesh.add_surface_from_arrays(1, a)
#	#if there's more than one surface, the surface idx should be the last
#	mesh.surface_set_material(0, m)
	
#	quadtree.setMeshInstanceId(id)

func build_mesh(mesh_data : Array, owner):
	if !mesh_data: return null

	var static_body : StaticBody = StaticBody.new()
	
	var id = mesh.get_surface_count()
	mesh.add_surface_from_arrays(ArrayMesh.PRIMITIVE_TRIANGLES, mesh_data[0])
	mesh.surface_set_material(id, material)
	
#	var polygon_shape : ConcavePolygonShape = ConcavePolygonShape.new()
#	polygon_shape.set_faces(mesh_data[1])
#	var owner_id = static_body.create_shape_owner(owner)
#	static_body.shape_owner_add_shape(owner_id, polygon_shape)
#
#	add_child(static_body)
	return id
