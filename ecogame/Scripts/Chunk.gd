class Chunk:
	extends Node
	
	var Voxel = load("res://Scripts/Voxel.gd")
	
	var _volume
	var _offset
	var _meshInstance
	var _intersectRef
	
	func _init(offset):
		self._offset = offset
		self._volume = null
		self._meshInstance = null
		self._intersectRef = funcref(self, "_intersection")
	
	func flatten_index(x, y, z):
		return x + WorldVariables.CHUNK_SIZE_X * (y + WorldVariables.CHUNK_SIZE_Y * z);
	
	func set_volume(volume):
		self._volume = volume
	
	func set_mesh_instance(meshInstance):
		self._meshInstance = meshInstance
	
	func get_volume():
		return self._volume
	
	func get_mesh_instance():
		return self._meshInstance
	
	func get_offset():
		return self._offset
	
	func get_voxel_v(vec):
		get_voxel(vec.x, vec.y, vec.z)
	
	func get_voxel(x, y, z):
		if x < 0 || x >= WorldVariables.CHUNK_SIZE_X: return 0
		if y < 0 || y >= WorldVariables.CHUNK_SIZE_Y: return 0
		if z < 0 || z >= WorldVariables.CHUNK_SIZE_Z: return 0
		return self._volume[self.flatten_index(x, y, z)]
	
	func set_voxel_v(vec, v):
		set_voxel(vec.x, vec.y, vec.z, v)
	
	func set_voxel(x, y, z, v):
		if x < 0 || x >= WorldVariables.CHUNK_SIZE_X: return
		if y < 0 || y >= WorldVariables.CHUNK_SIZE_Y: return
		if z < 0 || z >= WorldVariables.CHUNK_SIZE_Z: return
		self._volume[self.flatten_index(x, y, z)] = v
	
#	onready var intersectRef = funcref(self, "_intersection")
	func _intersection(x, y, z):
		var chunkX = self._offset[0]
		var chunkY = self._offset[1]
		var chunkZ = self._offset[2]
		if (x >= chunkX && x < chunkX + WorldVariables.CHUNK_SIZE_X
				&& y >= chunkY && y < chunkY + WorldVariables.CHUNK_SIZE_Y
				&& z >= chunkZ && z < chunkZ + WorldVariables.CHUNK_SIZE_Z):
			var voxel = self.get_voxel(
				int(x) % WorldVariables.CHUNK_SIZE_X,
				int(y) % WorldVariables.CHUNK_SIZE_Y,
				int(z) % WorldVariables.CHUNK_SIZE_Z)
			
			if voxel != 0:
				return Voxel.Voxel.new(Vector3(x, y, z), voxel, self)
		return null
	
	func get_voxel_ray(from, to):
		var list = []
		list = Intersection.segment3DIntersections(from, to, true, self._intersectRef, list)
		return null if list.empty() else list[0]
	
	func get_voxel_noise(x, y, z):
		var noise3DV = Vector3(x, y, z) * WorldVariables.NOISE_SCALE
		if Noise.noise_3d(noise3DV) / 2.0 + 0.5 > 0.5: return 0
		else: return 1
		
	func build_volume():
		_volume = PoolByteArray()
		_volume.resize(WorldVariables.CHUNK_SIZE_X * WorldVariables.CHUNK_SIZE_Y * WorldVariables.CHUNK_SIZE_Z)
		for z in WorldVariables.CHUNK_SIZE_Z:
			for y in WorldVariables.CHUNK_SIZE_Y:
				for x in WorldVariables.CHUNK_SIZE_X:
					_volume[flatten_index(x, y, z)] = get_voxel_noise(x + _offset[0], y + _offset[1], z + _offset[2])