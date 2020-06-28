extends VoxelWorld
class_name CelestialBody

# config
const TIME_PERIOD = 0.05 # 50ms
var time = 0
var size : int
var noise : OpenSimplexNoise
var center_of_gravity

onready var eco_game = get_tree().get_root().get_node("EcoGame")
var player

func _init(position, size, name).(position, size, WorldVariables.stoneMaterial, name):
	self.size = size
	center_of_gravity = Vector3(
		size * WorldVariables.CHUNK_SIZE / 2, 
		size * WorldVariables.CHUNK_SIZE / 2, 
		size * WorldVariables.CHUNK_SIZE / 2)
	
	noise = OpenSimplexNoise.new()
	noise.set_seed(WorldVariables.NOISE_SEED)
	noise.set_octaves(4)
	noise.set_period(64.0)
	noise.set_persistence(0.5)

func _ready():
	BehaviourGlobals.global_context.set("waypoint", center_of_gravity)
	player = eco_game.get_node("Player")
	voxel_body.buildChunks(to_local(player.global_transform.origin), 64.0)

func is_walkable(from : Vector3, to : Vector3, normal : Vector3):
#	var gravity = (center_of_gravity - to).normalized() * 8
#	var inv_normal = -normal
#	var angle = inv_normal.angle_to(gravity)
#	return angle <= PI / 3
	return true

func is_voxel(ix : int, iy : int, iz : int, offset : Vector3) -> float:
	if noise == null: return 0.0
	var cx = ix + offset.x
	var cy = iy + offset.y
	var cz = iz + offset.z
	var x = cx / (size * WorldVariables.CHUNK_SIZE)
	var y = cy / (size * WorldVariables.CHUNK_SIZE)
	var z = cz / (size * WorldVariables.CHUNK_SIZE)
	var d = 0.5
	var r = 0.08 * (noise.get_noise_3d(cx, cy, cz) * 0.5 + 0.5)
	var s = pow(x - d, 2) + pow(y - d, 2) + pow(z - d, 2) - r
#	s -= noise.get_noise_3d(cx, cy, cz) / 4
	return s

func get_center_of_gravity():
	return center_of_gravity

func _process(delta : float) -> void:
	time += delta
	if time > TIME_PERIOD:
		if voxel_body && player:
			pass
#			print (player.global_transform.origin)
#			voxel_body.buildChunks(to_local(player.global_transform.origin), 64.0)
		time = 0

#	var r = (PI / 128) * delta
#	var origin = global_transform.origin
#	origin = origin.rotated(Vector3(0, 1, 0), -r)
#	global_transform.origin = origin
#	rotate(Vector3(0, 0, 1), r*2)
