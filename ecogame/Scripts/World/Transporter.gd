extends WorldObject
class_name Transporter

var time = 0

func _init(position).(position):
	data.child_data = {
		"resources": {
			"iron": 0
		}
	}

# Called when the node enters the scene tree for the first time.
func _ready():
	pass # Replace with function body.

func _process(delta : float) -> void:
	if current_state == STATE.TRANSPORTING_IRON_TO_BASE:
		time += delta
		if time > 10.0:
			var amount_iron = data.child_data.resources.iron
#			emit_signal("storing_iron", self.get_instance_id(), amount_iron)
			print("transporter %s storing %s iron at base"%[self.get_instance_id(), amount_iron])
			data.child_data.resources.iron = 0
			set_state(STATE.RETURNING_TO_FACTORY)
			print("transporter %s returning to factory"%[self.get_instance_id()])
			time = 0
	
	if current_state == STATE.RETURNING_TO_FACTORY:
		time += delta
		if time > 10.0:
			print("transporter %s returned to factory"%[self.get_instance_id()])
			set_state(STATE.IDLE)
			time = 0

func transport_iron_to_base(amount_iron):
	data.child_data.resources.iron += amount_iron
	set_state(STATE.TRANSPORTING_IRON_TO_BASE)
	print("transporter %s transporting %s iron to base"%[self.get_instance_id(), amount_iron])
