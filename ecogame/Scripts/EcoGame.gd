extends Spatial
class_name EcoGame

var VoxelWorld = load("res://bin/VoxelWorld.gdns")

# build thread variables
const TIME_PERIOD = 0.05 # 50ms
const MAX_BUILD_CHUNKS = 24
const MAX_BUILD_SECTIONS = 1

# config
var time = 0
var build_stack : Array = []
var smooth_mat = SpatialMaterial.new()

func _ready() -> void:
	var voxelWorld = VoxelWorld.new()
	voxelWorld.setWidth(8)
	voxelWorld.setDepth(8)
	Lib.world = voxelWorld
	add_child(Lib.instance)
	add_child(Lib.world)
	Lib.game = self
	smooth_mat.albedo_color = Color(0.31, 0.46, 0.21)
	Lib.world.buildChunks()

func _process(delta : float) -> void:
	time += delta
	if time > TIME_PERIOD:
#		var player_pos = $Player.translation
		# Reset timer
		time = 0
	
	var w2 = (Lib.world.getWidth() * 16.0) * 0.5
	var d2 = (Lib.world.getDepth() * 16.0) * 0.5
	var pivot_point = Vector3() + Vector3(w2, 0, d2)
	var pivot_radius = Vector3() - pivot_point
	var pivot_transform = Transform(Lib.world.transform.basis, pivot_point)
	var r = (PI / 32) * delta
	Lib.world.transform = pivot_transform.rotated(Vector3(0, 1, 0), r).translated(pivot_radius)

func build_chunk(mesh_data : Array, chunk, world) -> void:
	if (!mesh_data || !chunk): return
	var old_mesh_instance_id = chunk.getMeshInstanceId()
	
	if old_mesh_instance_id != 0:
		var old_mesh_instance = instance_from_id(old_mesh_instance_id)
		if old_mesh_instance:
			world.remove_child(old_mesh_instance)
			old_mesh_instance.free()
	
	var mesh_instance = build_mesh_instance(mesh_data, chunk)
	Lib.world.add_child(mesh_instance)
	chunk.setMeshInstanceId(mesh_instance.get_instance_id())

func build_mesh_instance(mesh_data : Array, owner) -> MeshInstance:
	if (!mesh_data): return null
	
	var mesh_instance = MeshInstance.new()
	var mesh : ArrayMesh = ArrayMesh.new()
	var static_body : StaticBody = StaticBody.new()
	
	mesh.add_surface_from_arrays(ArrayMesh.PRIMITIVE_TRIANGLES, mesh_data[0])
	mesh.surface_set_material(0, smooth_mat)
	
	var polygon_shape : ConcavePolygonShape = ConcavePolygonShape.new()
	polygon_shape.set_faces(mesh_data[1])
	var owner_id = static_body.create_shape_owner(owner)
	static_body.shape_owner_add_shape(owner_id, polygon_shape)

	mesh_instance.add_child(static_body)
	mesh_instance.mesh = mesh
		
	return mesh_instance

func set_path_actor(path, actor_instance_id : int):
	var actor = instance_from_id(actor_instance_id)
	actor.get_node("BehaviourContext").set("path", path)

func draw_debug(geometry : ImmediateGeometry):
	var m = SpatialMaterial.new()
	m.flags_use_point_size = true
	m.vertex_color_use_as_albedo = true
	geometry.set_material_override(m)
	add_child(geometry)

func draw_debug_dots(geometry : ImmediateGeometry):
	var m = SpatialMaterial.new()
	m.flags_use_point_size = true
	m.vertex_color_use_as_albedo = true
	m.params_point_size = 4
	geometry.set_material_override(m)
	add_child(geometry)

