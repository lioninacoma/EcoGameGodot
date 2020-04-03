extends Spatial
class_name EcoGame

# build thread variables
const TIME_PERIOD = 0.05 # 50ms
const MAX_BUILD_CHUNKS = 24
const MAX_BUILD_SECTIONS = 1

# config
var time = 0
var build_stack : Array = []
var smooth_mat = SpatialMaterial.new()

func _ready() -> void:
	add_child(Lib.instance)
	Lib.game = self
	smooth_mat.albedo_color = Color(0.31, 0.46, 0.21)

func _process(delta : float) -> void:
	time += delta
	if time > TIME_PERIOD:
		var player_pos = $Player.translation
#		Lib.instance.buildSections(player_pos, WorldVariables.BUILD_DISTANCE, MAX_BUILD_SECTIONS)
		# Reset timer
		time = 0
	
	# starts ChunkBuilder jobs
#	process_build_stack()

func process_build_stack() -> void:
	for i in range(min(build_stack.size(), MAX_BUILD_CHUNKS)):
		var chunk = build_stack.pop_front()
		if chunk == null: continue
		Lib.instance.buildChunk(chunk, self)

func build_chunk_smooth(mesh_data : Array, chunk) -> void:
	if (!mesh_data || !chunk): return
	var old_mesh_instance_id = chunk.getMeshInstanceId()
	
	if old_mesh_instance_id != 0:
		var old_mesh_instance = instance_from_id(old_mesh_instance_id)
		if old_mesh_instance:
			remove_child(old_mesh_instance)
			old_mesh_instance.free()
	
	var mesh_instance = build_mesh_instance_smooth(mesh_data, chunk)
	add_child(mesh_instance)
	
	chunk.setMeshInstanceId(mesh_instance.get_instance_id())

func build_chunk(meshes : Array, chunk) -> void:
	if (!meshes || !chunk): return
	var old_mesh_instance_id = chunk.getMeshInstanceId()
	
	if old_mesh_instance_id != 0:
		var old_mesh_instance = instance_from_id(old_mesh_instance_id)
		if old_mesh_instance:
			remove_child(old_mesh_instance)
			old_mesh_instance.free()
	
	var mesh_instance = build_mesh_instance(meshes, chunk)
	add_child(mesh_instance)
	
	chunk.setMeshInstanceId(mesh_instance.get_instance_id())

func build_asset(meshes : Array, owner) -> Spatial:
	if (!meshes): return null
	
	var new_asset : Spatial = Spatial.new()
	var mesh_instance = MeshInstance.new()
	var mesh = ArrayMesh.new()
	var surface_index : int = 0
	var rigid_body : RigidBody = RigidBody.new()
	
	mesh_instance.name = "mesh"
	rigid_body.name = "body"
	
	rigid_body.add_child(mesh_instance)
	new_asset.add_child(rigid_body)
	
	for mesh_data in meshes:
		var mi = mesh_data[2] - 1
		if mesh_data[3] <= 0: continue
		
		mesh.add_surface_from_arrays(ArrayMesh.PRIMITIVE_TRIANGLES, mesh_data[0])
		mesh.surface_set_material(surface_index, WorldVariables.materials[mi])

		var polygon_shape : ConvexPolygonShape = ConvexPolygonShape.new()
		polygon_shape.set_points(mesh_data[1])
		var owner_id = rigid_body.create_shape_owner(owner)
		rigid_body.shape_owner_add_shape(owner_id, polygon_shape)

		surface_index += 1
	
	mesh_instance.mesh = mesh
	
	var weight = 16.0
	rigid_body.set_weight(9.8 * weight)
	rigid_body.set_mass(1.0)
	rigid_body.set_friction(1.0)
	rigid_body.set_bounce(0)
	rigid_body.set_can_sleep(true)
	
	return new_asset

func build_mesh_instance_smooth(mesh_data : Array, owner) -> MeshInstance:
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

func build_mesh_instance(meshes : Array, owner) -> MeshInstance:
	if (!meshes): return null
	
	var mesh_instance = MeshInstance.new()
	var mesh = ArrayMesh.new()
	var surface_index : int = 0
	var static_body : StaticBody = StaticBody.new()
	
	mesh_instance.name = "mesh"
	static_body.name = "body"
	
	for mesh_data in meshes:
		var mi = mesh_data[2] - 1
		if mesh_data[3] <= 0: continue
		
		mesh.add_surface_from_arrays(ArrayMesh.PRIMITIVE_TRIANGLES, mesh_data[0])
		mesh.surface_set_material(surface_index, WorldVariables.materials[mi])

		var polygon_shape : ConcavePolygonShape = ConcavePolygonShape.new()
		polygon_shape.set_faces(mesh_data[1])
		var owner_id = static_body.create_shape_owner(owner)
		static_body.shape_owner_add_shape(owner_id, polygon_shape)
		surface_index += 1
	
	mesh_instance.add_child(static_body)
	mesh_instance.mesh = mesh
	return mesh_instance

func build_chunk_queued(chunk):
	build_stack.push_front(chunk)

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

