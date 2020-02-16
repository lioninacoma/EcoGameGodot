extends Node

var task_handlers : Array = []

func add_handler(handler) -> void:
	task_handlers.push_back(handler)

func notify(event : TaskEvent):
	for handler in task_handlers:
		if event.handler == handler.get_instance_id(): continue
		handler.update(event)
