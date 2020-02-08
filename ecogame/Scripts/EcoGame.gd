extends Spatial
class_name EcoGame

var Actor : PackedScene = load("res://Actor.tscn")
var TaskManager = load("res://Scripts/Actor/Task/TaskManager.gd")

# build thread variables
const TIME_PERIOD = 0.2 # 200ms
const MAX_BUILD_CHUNKS = 24
const MAX_BUILD_SECTIONS = 1

onready var fps_label = get_node('FPSLabel')
var task_manager = TaskManager.new()

# config
var mouse_mode_captured : bool = false
var time = 0
var build_stack : Array = []

var amount_actors = 0
var actor
var asset = null
var asset_type : int = -1
var storehouse_location : Vector3 = Vector3()
var control_active : bool
var middle_pressed : bool = false
var middle_pressed_location : Vector3 = Vector3()

func _ready() -> void:
	Input.set_mouse_mode(Input.MOUSE_MODE_VISIBLE)
	add_child(Lib.instance)
	Lib.game = self

func _process(delta : float) -> void:
	fps_label.set_text(str(Engine.get_frames_per_second()))
	
	if middle_pressed:
		actor = Actor.instance()
		add_child(actor)
		actor.global_transform.origin.x = middle_pressed_location.x + 0.5
		actor.global_transform.origin.y = middle_pressed_location.y + 1
		actor.global_transform.origin.z = middle_pressed_location.z + 0.5
		if storehouse_location != Vector3(): 
			actor.gather_wood(storehouse_location)
		amount_actors += 1
		print("%s actors created."%[amount_actors])
	
	if Input.is_action_just_pressed("ui_cancel"):
		if mouse_mode_captured:
			Input.set_mouse_mode(Input.MOUSE_MODE_VISIBLE)
		else:
			Input.set_mouse_mode(Input.MOUSE_MODE_CAPTURED)
		mouse_mode_captured = !mouse_mode_captured
		
	time += delta
	if time > TIME_PERIOD:
		var player = $Player
		var pos = player.translation
		Lib.instance.buildSections(pos, WorldVariables.BUILD_DISTANCE, MAX_BUILD_SECTIONS)
		# Reset timer
		time = 0
	
	# starts ChunkBuilder jobs
	process_build_stack()

func process_build_stack() -> void:
	for i in range(min(build_stack.size(), MAX_BUILD_CHUNKS)):
		var chunk = build_stack.pop_front()
		if chunk == null: continue
		Lib.instance.buildChunk(chunk, self)

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
#	Lib.instance.updateGraph(chunk)

func build_mesh_instance_rigid(meshes : Array, owner) -> RigidBody:
	if (!meshes): return null
	
	var mesh_instance = MeshInstance.new()
	var mesh = ArrayMesh.new()
	var surface_index : int = 0
	var rigid_body : RigidBody = RigidBody.new()
	
	mesh_instance.name = "mesh"
	
	for mesh_data in meshes:
		var mi = mesh_data[2] - 1
		if mesh_data[3] <= 0: continue
		
		var polygon_shape : ConvexPolygonShape = ConvexPolygonShape.new()
		
		mesh.add_surface_from_arrays(ArrayMesh.PRIMITIVE_TRIANGLES, mesh_data[0])
		mesh.surface_set_material(surface_index, WorldVariables.materials[mi])

		polygon_shape.set_points(mesh_data[1])
		var owner_id = rigid_body.create_shape_owner(owner)
		rigid_body.shape_owner_add_shape(owner_id, polygon_shape)
		rigid_body.name = "body%s"%[surface_index]

		surface_index += 1
	
	rigid_body.add_child(mesh_instance)
	mesh_instance.mesh = mesh
	return rigid_body

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
		
		var polygon_shape : ConcavePolygonShape = ConcavePolygonShape.new()
		
		mesh.add_surface_from_arrays(ArrayMesh.PRIMITIVE_TRIANGLES, mesh_data[0])
		mesh.surface_set_material(surface_index, WorldVariables.materials[mi])

		polygon_shape.set_faces(mesh_data[1])
		var owner_id = static_body.create_shape_owner(owner)
		static_body.shape_owner_add_shape(owner_id, polygon_shape)
		surface_index += 1
	
	mesh_instance.add_child(static_body)
	mesh_instance.mesh = mesh
	return mesh_instance

func build_chunk_queued(chunk):
	build_stack.push_front(chunk)

func set_path_actor(path : PoolVector3Array, actor_instance_id : int):
	var actor = instance_from_id(actor_instance_id)
	actor.task_handler.set_task_data("path", path)

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

func _input(event : InputEvent) -> void:
	if event is InputEventKey:
		if event.scancode == KEY_CONTROL:
			control_active = event.pressed
	elif event is InputEventMouseMotion:
		var camera = $Player/Head/Camera
		var from = camera.project_ray_origin(event.position)
		var to = from + camera.project_ray_normal(event.position) * WorldVariables.PICK_DISTANCE
		var space_state = get_world().direct_space_state
		var result = space_state.intersect_ray(from, to)
		
		if result:
			var chunk = result.collider.shape_owner_get_owner(0)
			if not chunk: return
			
			var voxel_position = result.position
			var normal = result.normal
			var b = 0.1
			var vx = int(voxel_position.x - (b if normal.x > 0 else 0))
			var vy = int(voxel_position.y - (b if normal.y > 0 else 0))
			var vz = int(voxel_position.z - (b if normal.z > 0 else 0))
			
			middle_pressed_location.x = vx
			middle_pressed_location.y = vy
			middle_pressed_location.z = vz
			
			if asset:
				var w = asset.get_aabb().end.x - asset.get_aabb().position.x
				var d = asset.get_aabb().end.z - asset.get_aabb().position.z

				asset.global_transform.origin.x = vx - int(w / 2)
				asset.global_transform.origin.y = vy + 1
				asset.global_transform.origin.z = vz - int(d / 2)

				var fits = Lib.instance.voxelAssetFits(Vector3(vx, vy, vz), asset_type)
				for i in range(asset.get_surface_material_count()):
					var material = asset.mesh.surface_get_material(i)
					var color : Color
					if fits: color = Color(0, 1, 0)
					else:    color = Color(1, 0, 0)
					material.set_blend_mode(1)
					material.albedo_color = color
	elif event is InputEventMouseButton:
		var camera = $Player/Head/Camera
		var from = camera.project_ray_origin(event.position)
		var to = from + camera.project_ray_normal(event.position) * WorldVariables.PICK_DISTANCE
		var space_state = get_world().direct_space_state
		var result = space_state.intersect_ray(from, to)
		
		if event.button_index == BUTTON_MIDDLE:
			middle_pressed = event.pressed
		
		if result && event.pressed:
			var chunk = result.collider.shape_owner_get_owner(0)
			if !chunk: return
			
			var voxel_position = result.position
			var normal = result.normal
			var b = 0.1
			var vx = int(voxel_position.x - (b if normal.x > 0 else 0))
			var vy = int(voxel_position.y - (b if normal.y > 0 else 0))
			var vz = int(voxel_position.z - (b if normal.z > 0 else 0))
			
			if event.button_index == BUTTON_LEFT:
				if asset:
					var fits = Lib.instance.voxelAssetFits(Vector3(vx, vy, vz), asset_type)
					if !fits: return
					Lib.instance.addVoxelAsset(Vector3(vx, vy, vz), asset_type)
					var w = asset.get_aabb().end.x - asset.get_aabb().position.x
					var d = asset.get_aabb().end.z - asset.get_aabb().position.z
					storehouse_location.x = vx
					storehouse_location.y = vy
					storehouse_location.z = vz
					asset.queue_free()
					asset = null
					asset_type = -1
				else:
					asset_type = 1
					var meshes = Lib.instance.buildVoxelAssetByType(asset_type)
					asset = build_mesh_instance(meshes, null)
					var body = asset.get_node("body")
					asset.remove_child(body)
					
					for i in range(asset.get_surface_material_count()):
						var material = asset.mesh.surface_get_material(i).duplicate(true)
						material.set_blend_mode(1)
						material.albedo_color = Color(0, 1, 0)
						asset.mesh.surface_set_material(i, material)
					
					add_child(asset)
					var w = asset.get_aabb().end.x - asset.get_aabb().position.x
					var d = asset.get_aabb().end.z - asset.get_aabb().position.z
					asset.global_transform.origin.x = vx - int(w / 2)
					asset.global_transform.origin.y = vy + 1
					asset.global_transform.origin.z = vz - int(d / 2)
			elif event.button_index == BUTTON_RIGHT:
#				Lib.instance.setVoxel(Vector3(vx, vy, vz), 0)
#				if actor: actor.move_to(voxel_position, !control_active)

				actor = Actor.instance()
				add_child(actor)
				actor.global_transform.origin.x = middle_pressed_location.x + 0.5
				actor.global_transform.origin.y = middle_pressed_location.y + 1
				actor.global_transform.origin.z = middle_pressed_location.z + 0.5
				if storehouse_location != Vector3():
					actor.gather_wood(storehouse_location)
				amount_actors += 1
				print("%s actors created."%[amount_actors])