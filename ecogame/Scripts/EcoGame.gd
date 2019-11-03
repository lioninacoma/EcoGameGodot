extends Spatial
class_name EcoGame

onready var fpsLabel = get_node('FPSLabel')

# globals
onready var WorldVariables : Node = get_node("/root/WorldVariables")
onready var Intersection : Node = get_node("/root/Intersection")

# build thread variables
var chunkBuilder : ChunkBuilder
var chunks : Array = []
var buildStack : Array = []
var buildStackMaxSize : int = 30
var maxAmountBuildJobs : int = 8

# voxel variables
onready var intersectRef : FuncRef = funcref(self, "_intersection")

# config
var mouseModeCaptured : bool = true

func _ready() -> void:
	chunkBuilder = ChunkBuilder.new()
	add_child(chunkBuilder)
	Input.set_mouse_mode(Input.MOUSE_MODE_CAPTURED)

func _process(delta : float) -> void:
	fpsLabel.set_text(str(Engine.get_frames_per_second()))
	
	if Input.is_action_just_pressed("ui_cancel"):
		if mouseModeCaptured:
			Input.set_mouse_mode(Input.MOUSE_MODE_VISIBLE)
		else:
			Input.set_mouse_mode(Input.MOUSE_MODE_CAPTURED)
		mouseModeCaptured = !mouseModeCaptured
	
	# initializes chunks in range BUILD_DISTANCE to player
	initialize_chunks_nearby()
	# starts ChunkBuilder jobs
	process_build_stack()

func _input(event : InputEvent) -> void:
	if event is InputEventMouseButton and event.pressed and event.button_index == 1:
		var camera = $Player/Head/Camera
		var from = camera.project_ray_origin(event.position)
		var to = from + camera.project_ray_normal(event.position) * WorldVariables.PICK_DISTANCE
		var voxel = pick_voxel(from, to)
		if voxel != null:
			var chunk = voxel.get_chunk()
			var offset = chunk.get_offset()
			var position = voxel.get_position()
			var vx = int(position.x)
			var vy = int(position.y)
			var vz = int(position.z)
			chunk.set_voxel(
				vx % WorldVariables.CHUNK_SIZE_X,
				vy % WorldVariables.CHUNK_SIZE_Y,
				vz % WorldVariables.CHUNK_SIZE_Z, 0)
			var cx = int(offset.x) / WorldVariables.CHUNK_SIZE_X
			var cz = int(offset.y) / WorldVariables.CHUNK_SIZE_Z
			var index = flatten_index(cx, cz)
			buildStack.push_front(index)
			print("Voxel(%s, %s, %s) removed!"%[vx,vy,vz])

func initialize_chunks_nearby() -> void:
	if buildStack.size() >= buildStackMaxSize:
		return
	chunks.resize(WorldVariables.WORLD_SIZE * WorldVariables.WORLD_SIZE)
	
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
	xE = min(WorldVariables.WORLD_SIZE, xE)
	
	var zS = int(tl.z)
	zS = max(0, zS)
	
	var zE = int(br.z)
	zE = min(WorldVariables.WORLD_SIZE, zE)
	
	for z in range(zS, zE):
		for x in range(xS, xE):
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
			var chunk = Chunk.new(offset)
			add_child(chunk)
			chunks[index] = chunk
			buildStack.push_back(index)

func process_build_stack() -> void:
	for i in buildStack.size():
		if chunkBuilder.get_amount_active() < maxAmountBuildJobs:
			var index = buildStack.pop_front()
			if index < 0 || index >= chunks.size(): continue
			var chunk = chunks[index]
			if chunk == null: continue
			
			chunkBuilder.build_chunk(chunk)
#			print("%s started!" % [chunkBuilder.get_id()])
		else:
			break

func pick_voxel(from : Vector3, to : Vector3) -> Voxel:
	var voxel = null
	var dist = INF
	var chunks = get_chunks_ray(from, to)
	var voxels = []
	
	for chunk in chunks:
		var v = chunk.get_voxel_ray(from, to)
		if v != null:
			voxels.push_back(v)
	
	for v in voxels:
		var currDst = from.distance_to(v.get_position())
		if currDst < dist:
			dist = currDst
			voxel = v
	
	return voxel

func _intersection(x : int, y : int, z : int) -> Chunk:
	var xc = floor(x / WorldVariables.CHUNK_SIZE_X)
	var zc = floor(z / WorldVariables.CHUNK_SIZE_Z)
	if xc < 0 || xc >= WorldVariables.WORLD_SIZE: return null
	if zc < 0 || zc >= WorldVariables.WORLD_SIZE: return null
	var chunk = chunks[flatten_index(xc, zc)]
	if chunk != null:
		return chunk;
	return null

func get_chunks_ray(from : Vector3, to : Vector3) -> Array:
	var list = []
	list = Intersection.segment3DIntersections(from, to, false, intersectRef, list)
	return list

# ------------------------- HELPER FUNCTIONS -------------------------
func flatten_index(x : int, z : int) -> int:
	return x * WorldVariables.WORLD_SIZE + z

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