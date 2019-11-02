extends Node

var _noise
var _wr

func _ready():
	_init_noise()

func _init_noise():
	_noise = OpenSimplexNoise.new()
	_noise.seed = WorldVariables.noiseSeed
	_noise.octaves = 4
	_noise.period = 60.0
	_noise.persistence = 0.5
	_wr = weakref(_noise)

func _check_ref():
	if (!_wr.get_ref()):
		_init_noise();

func noise_2d(vec2D):
	_check_ref()
	return _noise.get_noise_2dv(vec2D)

func noise_3d(vec3D):
	_check_ref()
	return _noise.get_noise_3dv(vec3D)
