extends Spatial
class_name EcoGame

onready var fpsLabel = get_node('FPSLabel')

# globals
onready var WorldVariables : Node = get_node("/root/WorldVariables")

var Lib = load("res://bin/EcoGame.gdns").new()
var Chunk = load("res://bin/Chunk.gdns")
var Voxel = load("res://bin/Voxel.gdns")

# build thread variables
const TIME_PERIOD = 0.2 # 200ms
var time = 0
var buildStack : Array = []
var buildStackMaxSize : int = 20
var chunksBuild = 0

# config
var mouseModeCaptured : bool = true

func _ready() -> void:
	Input.set_mouse_mode(Input.MOUSE_MODE_CAPTURED)
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
		# starts ChunkBuilder jobs
		process_build_stack()
		# Reset timer
		time = 0

func process_build_stack() -> void:
	for i in range(buildStack.size()):
		var chunk = buildStack.pop_front()
		if chunk == null: continue
		chunk.setBuilding(true)
		Lib.buildChunk(chunk, self)

func build_mesh_instance(meshes : Array, chunk) -> void:
	if (!meshes || !chunk): return
	
	var meshInstance = MeshInstance.new()
	var mesh = ArrayMesh.new()
	var oldMeshInstanceId = chunk.getMeshInstanceId()
	
	for meshData in meshes:
		var mi = meshData[2] - 1
		if meshData[3] <= 0: continue
		var staticBody = StaticBody.new()
		var polygonShape = ConcavePolygonShape.new()
		
		mesh.add_surface_from_arrays(ArrayMesh.PRIMITIVE_TRIANGLES, meshData[0])
		mesh.surface_set_material(mi, WorldVariables.materials[mi])
		
		polygonShape.set_faces(meshData[1])
		var ownerId = staticBody.create_shape_owner(chunk)
		staticBody.shape_owner_add_shape(ownerId, polygonShape)
		meshInstance.add_child(staticBody)
	
	meshInstance.mesh = mesh
	
	if oldMeshInstanceId != 0:
		var oldMeshInstance = instance_from_id(oldMeshInstanceId)
		if oldMeshInstance:
			remove_child(oldMeshInstance)
			oldMeshInstance.free()
	
	add_child(meshInstance)
	chunk.setMeshInstanceId(meshInstance.get_instance_id())
	chunk.setBuilding(false)

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
				chunk.setVoxel(
					vx % WorldVariables.CHUNK_SIZE_X,
					vy % WorldVariables.CHUNK_SIZE_Y,
					vz % WorldVariables.CHUNK_SIZE_Z, 0)
				
				buildStack.push_front(chunk)