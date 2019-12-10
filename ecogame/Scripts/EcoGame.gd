extends Spatial
class_name EcoGame

onready var fpsLabel = get_node('FPSLabel')

# globals
onready var WorldVariables : Node = get_node("/root/WorldVariables")
onready var Intersection : Node = get_node("/root/Intersection")

var Lib = load("res://bin/EcoGame.gdns").new()
var Chunk = load("res://bin/Chunk.gdns")
var Voxel = load("res://bin/Voxel.gdns")

# build thread variables
const TIME_PERIOD = 0.2 # 200ms
var time = 0
var chunks : Array = []
var buildStack : Array = []
var buildStackMaxSize : int = 20
var chunksBuild = 0

# voxel variables
onready var intersectRef : FuncRef = funcref(self, "_intersection")

# config
var mouseModeCaptured : bool = true

func _ready() -> void:
	chunks.resize(WorldVariables.WORLD_SIZE * WorldVariables.WORLD_SIZE)
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
		# initializes chunks in range BUILD_DISTANCE to player
		initialize_chunks_nearby()
		# build assets
		build_areas()
		# starts ChunkBuilder jobs
		process_build_stack()
		# Reset timer
		time = 0

func build_areas() -> void:
	var player = $Player
	var pos = player.translation
	var indices : Array
	indices.clear()
	var d = 64
	var tl = Vector3(pos.x - d, 0, pos.z - d)
	var br = Vector3(pos.x + d, 0, pos.z + d)
	tl = to_chunk_coords(tl)
	br = to_chunk_coords(br)
	
	var xS = int(tl.x)
	xS = max(0, xS)
	
	var xE = int(br.x)
	xE = min(WorldVariables.WORLD_SIZE - 1, xE)
	
	var zS = int(tl.z)
	zS = max(0, zS)
	
	var zE = int(br.z)
	zE = min(WorldVariables.WORLD_SIZE - 1, zE)
	
	for z in range(zS, zE + 1):
		for x in range(xS, xE + 1):
			var index = flatten_index(x, z)
			var chunk = chunks[index]
			
			if !chunk || chunk.isAssetsBuilt(): 
				continue
			
			if buildStack.has(index) || !chunk.isReady():
				return;
			
			indices.push_back(index)
	
	if !indices.empty():
		Lib.buildAreas(indices, tl, br)

func initialize_chunks_nearby() -> void:
	if buildStack.size() >= buildStackMaxSize:
		return
	
	var player = $Player
	var pos = player.translation
	
	var d = WorldVariables.BUILD_DISTANCE + 10
	var tl = Vector3(pos.x - d, 0, pos.z - d)
	var br = Vector3(pos.x + d, 0, pos.z + d)
	tl = to_chunk_coords(tl)
	br = to_chunk_coords(br)
	
	var xS = int(tl.x)
	xS = max(0, xS)
	
	var xE = int(br.x)
	xE = min(WorldVariables.WORLD_SIZE - 1, xE)
	
	var zS = int(tl.z)
	zS = max(0, zS)
	
	var zE = int(br.z)
	zE = min(WorldVariables.WORLD_SIZE - 1, zE)
	
	for z in range(zS, zE + 1):
		for x in range(xS, xE + 1):
			if buildStack.size() >= buildStackMaxSize:
				return
			var index = flatten_index(x, z)
			var centerX = x * WorldVariables.CHUNK_SIZE_X + WorldVariables.CHUNK_SIZE_X / 2.0;
			var centerZ = z * WorldVariables.CHUNK_SIZE_Z + WorldVariables.CHUNK_SIZE_Z / 2.0;
			
			if pos.distance_to(Vector3(centerX, pos.y, centerZ)) > WorldVariables.BUILD_DISTANCE:
				continue
			
			if chunks[index]: 
				continue
			
			var offset = Vector3(x * WorldVariables.CHUNK_SIZE_X, 0, z * WorldVariables.CHUNK_SIZE_Z)
			var chunk = Chunk.new()
			chunk.setOffset(offset)
			chunks[index] = chunk
			buildStack.push_back(index)

func process_build_stack() -> void:
	for i in range(buildStack.size()):
		var index = buildStack.pop_front()
		if index < 0 || index >= chunks.size(): continue
		var chunk = chunks[index]
		if chunk == null: continue
		chunk.setBuilding(true)
		Lib.buildChunk(chunk, self)

func build_mesh_instance(meshes : Array, chunk) -> void:
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
				var cx = int(offset.x) / WorldVariables.CHUNK_SIZE_X
				var cz = int(offset.z) / WorldVariables.CHUNK_SIZE_Z
				var index = flatten_index(cx, cz)
	
				chunk.setVoxel(
					vx % WorldVariables.CHUNK_SIZE_X,
					vy % WorldVariables.CHUNK_SIZE_Y,
					vz % WorldVariables.CHUNK_SIZE_Z, 0)
	
				buildStack.push_front(index)
#			else:
#				var center : Vector2 = Vector2(vx, vz)
#				var radius : float = 48.0
#
#				Lib.buildAreas(center, radius)

func pick_voxel(from : Vector3, to : Vector3):
	var voxel = null
	var dist = INF
	var chunks = get_chunks_ray(from, to)

	var voxels = []

	for chunk in chunks:
		var v = chunk.getVoxelRay(from, to)
		if v != null:
			voxels.push_back(v)

	for v in voxels:
		var currDst = from.distance_to(v.getPosition())
		if currDst < dist:
			dist = currDst
			voxel = v
			
	return voxel

func _intersection(x : int, y : int, z : int):
	var cx = floor(x / WorldVariables.CHUNK_SIZE_X)
	var cz = floor(z / WorldVariables.CHUNK_SIZE_Z)
	if cx < 0 || cx >= WorldVariables.WORLD_SIZE: return null
	if cz < 0 || cz >= WorldVariables.WORLD_SIZE: return null
	var chunk = get_chunk(cx, cz)
	if chunk != null:
		return chunk;
	return null

func get_chunks_ray(from : Vector3, to : Vector3) -> Array:
	var list = []
	list = Intersection.segment3DIntersections(from, to, false, intersectRef, list)
	return list

func get_chunk(x : int, z : int):
	if x < 0 || x >= WorldVariables.WORLD_SIZE: return null
	if z < 0 || z >= WorldVariables.WORLD_SIZE: return null
	var chunk = chunks[flatten_index(x, z)]
	return chunk

func get_chunk_by_index(i : int):
	if i < 0 || i >= chunks.size(): return null
	var chunk = chunks[i]
	return chunk

func update_chunks(indices : Array, size : int) -> void:
	for index in indices:
		buildStack.push_back(index)

# ------------------------- HELPER FUNCTIONS -------------------------
func flatten_index(x : int, z : int) -> int:
	return x + z * WorldVariables.WORLD_SIZE

func to_chunk_coords(position : Vector3) -> Vector3:
	var ix = int(position.x)
	var iy = int(position.y)
	var iz = int(position.z)
	var chunkX = ix / WorldVariables.CHUNK_SIZE_X
	var chunkY = iy / WorldVariables.CHUNK_SIZE_Y
	var chunkZ = iz / WorldVariables.CHUNK_SIZE_Z
	position.x = chunkX
	position.y = chunkY
	position.z = chunkZ
	return position