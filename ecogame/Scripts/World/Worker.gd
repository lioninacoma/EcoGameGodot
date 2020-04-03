extends WorldObject
class_name Worker

signal storing_iron(id, amount)

var time = 0

func _init(position).(position):
	pass

# Called when the node enters the scene tree for the first time.
func _ready():
	pass # Replace with function body.

func _process(delta : float) -> void:
	time += delta
	if time > 1.0 && current_state == STATE.GATHERING_IRON:
		emit_signal("storing_iron", self.get_instance_id(), 10)
		time = 0

func gather_iron(iron_mine):
	set_state(STATE.GATHERING_IRON)
