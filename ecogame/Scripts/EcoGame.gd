extends Spatial
class_name EcoGame

onready var fpsLabel = get_node('FPSLabel')

# globals
onready var WorldVariables : Node = get_node("/root/WorldVariables")

var Lib = load("res://bin/EcoGame.gdns").new()
var Actor : PackedScene = load("res://Actor.tscn")

# build thread variables
const TIME_PERIOD = 0.2 # 200ms
var time = 0
var buildStack : Array = []
var maxChunksBuild : int = 16

# config
var mouseModeCaptured : bool = false

func _ready() -> void:
	Input.set_mouse_mode(Input.MOUSE_MODE_VISIBLE)
	add_child(Lib)

func _process(delta : float) -> void:
	fpsLabel.set_text(str(Engine.get_frames_per_second()))
	
	if Input.is_action_just_pressed("ui_cancel"):
		if mouseModeCaptured:
			Input.set_mouse_mode(Input.MOUSE_MODE_VISIBLE)
		else:
			Input.set_mouse_mode(Input.MOUSE_MODE_CAPTURED)
		mouseModeCaptured = !mouseModeCaptured
		
	time += delta
	if time > TIME_PERIOD:
		var player = $Player
		var pos = player.translation
		var d = 256
		Lib.buildSections(pos, d)
		# Reset timer
		time = 0
	
	# starts ChunkBuilder jobs
	process_build_stack()

func process_build_stack() -> void:
	for i in range(min(buildStack.size(), maxChunksBuild)):
		var chunk = buildStack.pop_front()
		if chunk == null: continue
		chunk.setBuilding(true)
		Lib.buildChunk(chunk, self)

func build_mesh_instance(meshes : Array, chunk) -> void:
	if (!meshes || !chunk): return
	
	var meshInstance = MeshInstance.new()
	var mesh = ArrayMesh.new()
	var oldMeshInstanceId = chunk.getMeshInstanceId()
	var surfaceIndex = 0
	
	for meshData in meshes:
		var mi = meshData[2] - 1
		if meshData[3] <= 0: continue
		var staticBody : StaticBody = StaticBody.new()
		var polygonShape : ConcavePolygonShape = ConcavePolygonShape.new()
		
		mesh.add_surface_from_arrays(ArrayMesh.PRIMITIVE_TRIANGLES, meshData[0])
		mesh.surface_set_material(surfaceIndex, WorldVariables.materials[mi])
		
		polygonShape.set_faces(meshData[1])
		var ownerId = staticBody.create_shape_owner(chunk)
		staticBody.shape_owner_add_shape(ownerId, polygonShape)
		staticBody.name = "sb"
		meshInstance.add_child(staticBody)
		
		surfaceIndex += 1
	
	meshInstance.mesh = mesh
	
	if oldMeshInstanceId != 0:
		var oldMeshInstance = instance_from_id(oldMeshInstanceId)
		if oldMeshInstance:
			remove_child(oldMeshInstance)
			oldMeshInstance.free()
	
	add_child(meshInstance)
	chunk.setMeshInstanceId(meshInstance.get_instance_id())
	chunk.setBuilding(false)
	Lib.updateGraph(chunk)

func build_chunk_queued(chunk):
	buildStack.push_front(chunk)

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
	m.params_point_size = 3
	geometry.set_material_override(m)
	add_child(geometry)

var actor : Actor

func _input(event : InputEvent) -> void:
	if event is InputEventMouseButton and event.pressed:
		var camera = $Player/Head/Camera
		var from = camera.project_ray_origin(event.position)
		var to = from + camera.project_ray_normal(event.position) * WorldVariables.PICK_DISTANCE
		var space_state = get_world().direct_space_state
		var result = space_state.intersect_ray(from, to)
		
		if result:
			var chunk = result.collider.shape_owner_get_owner(0)
			var offset = chunk.getOffset()
			var voxelPosition = result.position
			
			var normal = result.normal
			var b = 0.1
			var vx = int(voxelPosition.x - (b if normal.x > 0 else 0))
			var vy = int(voxelPosition.y - (b if normal.y > 0 else 0))
			var vz = int(voxelPosition.z - (b if normal.z > 0 else 0))
			
			if event.button_index == 1:
				pass
			elif event.button_index == 2:
				
				if actor:
					var navStart = actor.global_transform.origin
					var navEnd = voxelPosition
					var path : PoolVector3Array = Lib.navigate(navStart, navEnd)
					actor.follow_path(path)
			
			elif event.button_index == 3:
				actor = Actor.instance()
				add_child(actor)
				actor.global_transform.origin.x = vx + 0.5
				actor.global_transform.origin.y = vy + 1
				actor.global_transform.origin.z = vz + 0.5
				
#				chunk.setVoxel(
#					vx % WorldVariables.CHUNK_SIZE_X,
#					vy % WorldVariables.CHUNK_SIZE_Y + 1,
#					vz % WorldVariables.CHUNK_SIZE_Z, 1)
#
#				build_chunk_queued(chunk)