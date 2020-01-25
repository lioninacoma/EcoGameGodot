extends Node

var taskHandlers : Array = []

func add_handler(handler) -> void:
	taskHandlers.push_back(handler)

func notify(event : TaskEvent):
	for handler in taskHandlers:
		if handler.get_instance_id() == event.handler.get_instance_id(): continue
		handler.update(event)