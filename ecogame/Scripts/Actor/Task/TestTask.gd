extends Task

var counter = 1

func _init():
	self.init("Test")

func perform(delta : float, actor : Actor) -> bool:
	if counter > 100:
		return true
	print("performing task %s %s/100"%[taskName, counter])
	if counter % 10 == 0:
#		var path = Lib.instance.navigate(Vector3(0, 0, 0), Vector3(10, 0, 10))
		self.notify(["test"], {
			"global": true, 
			"receiver": actor.get_instance_id(), 
			"receiver_class": "Actor"})
	counter += 1
	return false

func update(event : TaskEvent) -> void:
	pass