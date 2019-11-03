extends Node
class_name Voxel

var _position : Vector3
var _type : int
var _chunk

func _init(position : Vector3, type : int, chunk) -> void:
	self._position = position
	self._type = type
	self._chunk = chunk

func get_position() -> Vector3:
	return _position

func get_type() -> int:
	return _type

func get_chunk():
	return _chunk
	