extends Spatial

var ChunkBuilderThread = load("res://Scripts/ChunkBuilderThread.gd")
var Chunk = load("res://Scripts/Chunk.gd")

onready var fpsLabel = get_node('FPSLabel')

# build thread variables
var chunks = []
var buildStack = []
var maxAmountBuildJobs = 4
var amountBuildJobs = 0

# config
var mouseModeCaptured = true

func _ready():
	Input.set_mouse_mode(Input.MOUSE_MODE_CAPTURED)
	initialize_chunks()

func flatten_index(x, z):
	return x * WorldVariables.WORLD_SIZE + z

func initialize_chunks():
	# generate world map
	chunks.resize(WorldVariables.WORLD_SIZE * WorldVariables.WORLD_SIZE)
	for z in WorldVariables.WORLD_SIZE:
		for x in WorldVariables.WORLD_SIZE:
			var offset = [x * WorldVariables.CHUNK_SIZE_X, 0, z * WorldVariables.CHUNK_SIZE_Z]
			var index = flatten_index(x, z)
			chunks[index] = Chunk.Chunk.new(offset)
			buildStack.push_back(index)

func _process(delta):
	fpsLabel.set_text(str(Engine.get_frames_per_second()))
	
	if Input.is_action_just_pressed("ui_cancel"):
		if mouseModeCaptured:
			Input.set_mouse_mode(Input.MOUSE_MODE_VISIBLE)
		else:
			Input.set_mouse_mode(Input.MOUSE_MODE_CAPTURED)
		mouseModeCaptured = !mouseModeCaptured
	
	for i in buildStack.size():
		if amountBuildJobs < maxAmountBuildJobs:
			var index = buildStack.pop_front()
			if index < 0 || index >= chunks.size(): continue
			var chunk = chunks[index]
			if chunk == null: continue
			
			var chunkBuilder = ChunkBuilderThread.ChunkBuilderThread.new(chunk)
			add_child(chunkBuilder)
			chunkBuilder.generate_mesh()
			amountBuildJobs += 1
			
			print("%s started!" % [chunkBuilder.get_id()])
		else:
			break

func build_thread_ready(chunkBuilder):
	amountBuildJobs -= 1
	remove_child(chunkBuilder)
	print("%s finished!" % [chunkBuilder.get_id()])

func _input(event):
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
			var cx = offset[0] / WorldVariables.CHUNK_SIZE_X
			var cz = offset[2] / WorldVariables.CHUNK_SIZE_Z
			var index = flatten_index(cx, cz)
			buildStack.push_front(index)
			print("Voxel(%s, %s, %s) removed!"%[vx,vy,vz])

func pick_voxel(from, to):
	var voxel = null
	var dist = WorldVariables.MAX_INT
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

onready var intersectRef = funcref(self, "_intersection")
func _intersection(x, y, z):
	var xc = floor(x / WorldVariables.CHUNK_SIZE_X)
	var zc = floor(z / WorldVariables.CHUNK_SIZE_Z)
	if xc < 0 || xc >= WorldVariables.WORLD_SIZE: return null
	if zc < 0 || zc >= WorldVariables.WORLD_SIZE: return null
	var chunk = chunks[flatten_index(xc, zc)]
	if chunk != null:
		return chunk;
	return null

func get_chunks_ray(from, to):
	var list = []
	list = Intersection.segment3DIntersections(from, to, false, intersectRef, list)
	return list