extends VoxelWorld
class_name IronVein

var size : int
var noise : OpenSimplexNoise

func _init(position, size).(position, Vector3(size, size, size), WorldVariables.dirtMaterial):
	self.size = size
	
	noise = OpenSimplexNoise.new()
	noise.set_seed(WorldVariables.NOISE_SEED)
	noise.set_octaves(2)
	noise.set_period(22.0)
	noise.set_persistence(0.5)

func is_voxel(ix : int, iy : int, iz : int, offset : Vector3) -> float:
	if noise == null: return 0.0
	var d = 0.5
	var cx = ix + offset.x
	var cy = iy + offset.y
	var cz = iz + offset.z
	var x = cx / (size * WorldVariables.CHUNK_SIZE_X)
	var y = cy / (size * WorldVariables.CHUNK_SIZE_X)
	var z = cz / (size * WorldVariables.CHUNK_SIZE_X)
	var s = pow(x - d, 2) + pow(y - d, 2) + pow(z - d, 2) + 0.05
	s += noise.get_noise_3d(cx, cy, cz) / 2
	return s
