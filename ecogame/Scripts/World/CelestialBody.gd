extends VoxelWorld
class_name CelestialBody

# config
const TIME_PERIOD = 0.5 # 500ms
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
	voxel_body.build()
#	voxel_body.buildChunks(to_local(player.global_transform.origin), 64.0)

func get_center_of_gravity():
	return center_of_gravity

func _process(delta : float) -> void:
	time += delta
	if time > TIME_PERIOD:
		if voxel_body && player:
			pass
			voxel_body.update(to_local(player.global_transform.origin))
#			print (player.global_transform.origin)
#			voxel_body.buildChunks(to_local(player.global_transform.origin), 64.0)
		time = 0
