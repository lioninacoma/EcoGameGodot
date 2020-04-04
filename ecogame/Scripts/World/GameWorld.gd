extends Node
class_name GameWorld

# Called when the node enters the scene tree for the first time.
func _ready():
#	var spacecraft = Spacecraft.new()
#	add_object(spacecraft)
#	var base = Base.new(Vector3())
#	add_object(base)
#	
#	var dict = {
#		"test": 123,
#		"abc": {
#			"foo": "bar"
#		}
#	}
#	# save
#	var data = JSON.print(dict)
#	var file = File.new()
#	file.open("res://test.dat", File.WRITE)
#	file.store_string(data)
#	file.close()
#	# load
#	file = File.new()
#	file.open("res://test.dat", File.READ)
#	var content = file.get_as_text()
#	file.close()
#	var res = JSON.parse(content).result
	pass

func add_object(object):
	add_child(object)
	object.world = self
	object.connect("changing_state", self, "_on_object_changing_state")

func _on_object_changing_state(id, state) -> void:
#	print("object %s changing state to %s"%[id, state])
	pass
