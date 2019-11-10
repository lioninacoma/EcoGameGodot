extends Spatial
class_name EcoGame

onready var fpsLabel = get_node('FPSLabel')

# globals
onready var WorldVariables : Node = get_node("/root/WorldVariables")
onready var Intersection : Node = get_node("/root/Intersection")

var Lib = load("res://bin/EcoGame.gdns").new()
var Chunk = load("res://bin/Chunk.gdns")

# build thread variables
var chunks : Array = []
var buildStack : Array = []
var buildStackMaxSize : int = 30
var maxAmountBuildJobs : int = 8

# voxel variables
onready var intersectRef : FuncRef = funcref(self, "_intersection")

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
	
	# initializes chunks in range BUILD_DISTANCE to player
	initialize_chunks_nearby()
	# starts ChunkBuilder jobs
	process_build_stack()

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
			var chunk = Chunk.new()
			chunk.setOffset(offset)
			add_child(chunk)
			chunks[index] = chunk
			buildStack.push_back(index)

func process_build_stack() -> void:
	for i in buildStack.size():
		if i > 8: break
		var index = buildStack.pop_front()
		if index < 0 || index >= chunks.size(): continue
		var chunk = chunks[index]
		if chunk == null: continue
#		print(self.get_class())
		Lib.buildChunk(chunk, self)

func addMeshInstance(meshIntance : MeshInstance) -> void:
#	print("SET MESH INSTANCE")
	self.add_child(meshIntance)

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