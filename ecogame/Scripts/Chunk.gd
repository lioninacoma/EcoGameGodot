extends Node
class_name Chunk

# globals
onready var WorldVariables = get_node("/root/WorldVariables")
onready var Intersection = get_node("/root/Intersection")
onready var Noise = get_node("/root/Noise")

var _offset : Vector3
var _volume : PoolByteArray
var _meshInstance : MeshInstance
var _intersectRef : FuncRef

func _init(offset : Vector3) -> void:
	self._offset = offset
	self._intersectRef = funcref(self, "_intersection")

func flatten_index(x : int, y : int, z : int) -> int:
	return x + WorldVariables.CHUNK_SIZE_X * (y + WorldVariables.CHUNK_SIZE_Y * z);

func position(i : int) -> Vector3:
	return Vector3(
		i % WorldVariables.CHUNK_SIZE_X,
	    i / (WorldVariables.CHUNK_SIZE_X * WorldVariables.CHUNK_SIZE_Z),
	    (i / WorldVariables.CHUNK_SIZE_X) % WorldVariables.CHUNK_SIZE_Z)

func set_volume(volume : PoolByteArray) -> void:
	self._volume = volume

func set_mesh_instance(meshInstance : MeshInstance) -> void:
	self._meshInstance = meshInstance

func get_volume() -> PoolByteArray:
	return self._volume

func get_mesh_instance() -> MeshInstance:
	return self._meshInstance

func get_offset() -> Vector3:
	return self._offset

func get_voxel_v(vec : Vector3) -> int:
	return get_voxel(int(vec.x), int(vec.y), int(vec.z))

func get_voxel(x : int, y : int, z : int) -> int:
	if x < 0 || x >= WorldVariables.CHUNK_SIZE_X: return 0
	if y < 0 || y >= WorldVariables.CHUNK_SIZE_Y: return 0
	if z < 0 || z >= WorldVariables.CHUNK_SIZE_Z: return 0
	return _volume[flatten_index(x, y, z)]

func set_voxel_v(vec : Vector3, v : int) -> void:
	set_voxel(int(vec.x), int(vec.y), int(vec.z), v)

func set_voxel(x : int, y : int, z : int, v : int) -> void:
	if x < 0 || x >= WorldVariables.CHUNK_SIZE_X: return
	if y < 0 || y >= WorldVariables.CHUNK_SIZE_Y: return
	if z < 0 || z >= WorldVariables.CHUNK_SIZE_Z: return
	_volume[flatten_index(x, y, z)] = v

func set_voxel_column_v(vec : Vector3, v : int) -> void:
	set_voxel_column(int(vec.x), int(vec.y), int(vec.z), v)

func set_voxel_column(x : int, y : int, z : int, v : int) -> void:
	for i in y:
		set_voxel(x, i, z, v)

func _intersection(x : int, y : int, z : int) -> Voxel:
	var chunkX = _offset[0]
	var chunkY = _offset[1]
	var chunkZ = _offset[2]
	if (x >= chunkX && x < chunkX + WorldVariables.CHUNK_SIZE_X
			&& y >= chunkY && y < chunkY + WorldVariables.CHUNK_SIZE_Y
			&& z >= chunkZ && z < chunkZ + WorldVariables.CHUNK_SIZE_Z):
		var voxel = get_voxel(
			int(x) % WorldVariables.CHUNK_SIZE_X,
			int(y) % WorldVariables.CHUNK_SIZE_Y,
			int(z) % WorldVariables.CHUNK_SIZE_Z)
		
		if voxel != 0:
			return Voxel.new(Vector3(x, y, z), voxel, self)
	return null

func get_voxel_ray(from : Vector3, to : Vector3) -> Voxel:
	var list = []
	list = Intersection.segment3DIntersections(from, to, true, _intersectRef, list)
	return null if list.empty() else list[0]

func get_voxel_noise_y(x : int, z : int) -> int:
	var noise2DV = Vector2(x + _offset[0], z + _offset[2]) * 0.25
	var y = Noise.noise_2d(noise2DV) / 2.0 + 0.5
	y = redistribution(y, 4.0)
	y *= WorldVariables.CHUNK_SIZE_Y
	return int(y)

func get_voxel_noise_chance(x : int, y : int, z : int) -> float:
	var noise3DV = Vector3(x + _offset[0], y + _offset[1], z + _offset[2]) * WorldVariables.NOISE_SCALE
	return Noise.noise_3d(noise3DV) / 2.0 + 0.5

func cot(x : float) -> float:
	return 1.0 / tan(x)

func redistribution(x : float, r : float) -> float:
	var d = 1.0 + exp(cot(10.0 * x / PI) * r)
	return 1.0 / d

func build_volume() -> PoolByteArray:
	_volume = PoolByteArray()
	_volume.resize(WorldVariables.CHUNK_SIZE_X * WorldVariables.CHUNK_SIZE_Y * WorldVariables.CHUNK_SIZE_Z)
	
	for i in _volume.size():
		_volume[i] = 1
	
#	for i in _volume.size():
#		_volume[i] = 0
#
#	for z in WorldVariables.CHUNK_SIZE_Z:
#		for x in WorldVariables.CHUNK_SIZE_X:
#			var y = get_voxel_noise_y(x, z)
#			for i in y:
#				var c = get_voxel_noise_chance(x, i, z)
#				var t = 0.5
#				set_voxel(x, i, z, 1)
	
	return _volume
	