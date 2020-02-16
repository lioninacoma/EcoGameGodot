extends Node
class_name BehaviourContext

var data : Dictionary = {}

func set(key : String, value):
	data[key] = value

func get(key : String):
	return data[key] if data.has(key) else null
