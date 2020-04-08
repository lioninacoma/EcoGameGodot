extends WorldObject
class_name CelestialBody

var VoxelWorld = load("res://bin/VoxelWorld.gdns")

# config
const TIME_PERIOD = 0.05 # 50ms
var time = 0
var voxel_body

func _init(position, size).(position):
	voxel_body = VoxelWorld.new()
	voxel_body.setDimensions(Vector2(size, size))
	add_child(voxel_body)
	
	var s2 = size * 0.5 * WorldVariables.CHUNK_SIZE_X
	voxel_body.global_translate(-Vector3(s2, s2, s2))
	global_translate(position)

func _ready():
	voxel_body.buildChunks()

func _process(delta : float) -> void:
	time += delta
	if time > TIME_PERIOD:
		time = 0
	
	var r = (PI / 128) * delta
	var origin = global_transform.origin
	origin = origin.rotated(Vector3(0, 1, 0), -r)
	global_transform.origin = origin
	rotate(Vector3(0, 0, 1), r*2)
