class Voxel:
	extends Node
	
	var _position
	var _type
	var _chunk
	
	func _init(position, type, chunk):
		self._position = position
		self._type = type
		self._chunk = chunk
	
	func get_position():
		return _position
	
	func get_type():
		return _type
	
	func get_chunk():
		return _chunk
	