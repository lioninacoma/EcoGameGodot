extends Task

func _init():
	self.init("Observer")

func perform(delta : float, actor : Actor) -> bool:
	return false

func update(event : TaskEvent) -> void:
	print("%s/%s -> %s/%s"%[event.handler.get_instance_id(), event.task.get_instance_id(), taskHandler.get_instance_id(), self.get_instance_id()])
	pass
