extends Node
class_name WorldObject

signal changing_state(id, state)

var data
var position = Vector3()
var current_state = null
var world

enum STATE {
	IDLE,
	# Worker
	GATHERING_IRON
}

func _init(position):
	data = {
		"position": position,
		"current_state": null,
		"world": null,
		"child_data": null
	}
	self.position = position

# Called when the node enters the scene tree for the first time.
func _ready():
	pass

func _process(delta : float) -> void:
	if current_state == null:
		set_state(STATE.IDLE)

func set_state(state):
	emit_signal("changing_state", self.get_instance_id(), state)
	current_state = state
	data.current_state = state

func set_position(position):
	self.position = position
	data.position = position

func get_position():
	return self.position

func get_world():
	if world == null:
		world = get_tree().get_root().get_node("EcoGame").get_node("World")
		data.position = world.get_instance_id()
	return world
