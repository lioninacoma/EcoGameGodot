extends WorldObject
class_name CelestialBody

var VoxelWorld = load("res://bin/VoxelWorld.gdns")

# config
const TIME_PERIOD = 0.05 # 50ms
var time = 0
var voxel_body
var size : int
var noise : OpenSimplexNoise

func _init(position, size).(position):
	noise = OpenSimplexNoise.new()
	noise.set_seed(WorldVariables.NOISE_SEED)
	noise.set_octaves(6)
	noise.set_period(192.0)
	noise.set_persistence(0.5)
	
	self.size = size
	voxel_body = VoxelWorld.new()
	voxel_body.setDimensions(Vector2(size, size))
	voxel_body.setIsVoxelFn(funcref(self, "is_voxel"))
	add_child(voxel_body)
	
	var s2 = size * 0.5 * WorldVariables.CHUNK_SIZE_X
	voxel_body.global_translate(-Vector3(s2, s2, s2))
	global_translate(position)

func _ready():
	voxel_body.buildChunks()

func is_voxel(ix : int, iy : int, iz : int, offset : Vector3) -> float:
	if noise == null: return 0.0
	var d = 0.5
	var cx = ix + offset.x
	var cy = iy + offset.y
	var cz = iz + offset.z
	var x = cx / (size * WorldVariables.CHUNK_SIZE_X)
	var y = cy / (size * WorldVariables.CHUNK_SIZE_X)
	var z = cz / (size * WorldVariables.CHUNK_SIZE_X)
	var s = pow(x - d, 2) + pow(y - d, 2) + pow(z - d, 2) - 0.1;
	s += noise.get_noise_3d(cx, cy, cz) / 8;
	return s

func _process(delta : float) -> void:
	time += delta
	if time > TIME_PERIOD:
		time = 0
	
	var r = (PI / 128) * delta
	var origin = global_transform.origin
	origin = origin.rotated(Vector3(0, 1, 0), -r)
	global_transform.origin = origin
	rotate(Vector3(0, 0, 1), r*2)
