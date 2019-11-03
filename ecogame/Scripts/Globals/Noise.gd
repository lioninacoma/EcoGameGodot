extends Node

onready var WorldVariables : Node = get_node("/root/WorldVariables")

var _noise : OpenSimplexNoise
var _wr : WeakRef

func _ready() -> void:
	_init_noise()
	
func _init_noise() -> void:
	_noise = OpenSimplexNoise.new()
	_noise.seed = WorldVariables.NOISE_SEED
	_noise.octaves = 4
	_noise.period = 60.0
	_noise.persistence = 0.5
	_wr = weakref(_noise)

func noise_2d(vec2D : Vector2) -> float:
	if !_wr.get_ref():
		print("WEAK REF LOST")
		_init_noise()
	return _noise.get_noise_2dv(vec2D)

func noise_3d(vec3D : Vector3) -> float:
	if !_wr.get_ref():
		print("WEAK REF LOST")
		_init_noise()
	return _noise.get_noise_3dv(vec3D)
