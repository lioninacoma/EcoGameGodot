extends Task
class_name TaskSeries

var tasks : Array = []
var loop : bool = false
var i = 0

func interrupt():
	.interrupt()
	for task in tasks:
		task.interrupt()
	loop = false

func set_loop(loop : bool):
	self.loop = loop

func _init():
	self.init("TaskSeries")

func perform(delta : float, actor) -> bool:
	if !tasks.empty():
		var task = tasks[i]
		if task.interrupted || task.perform(delta, actor):
			i += 1
		
		if i == tasks.size():
			i = 0
			return !loop
		return false
		
	return true

func update(event : TaskEvent) -> void:
	.update(event)
	for task in tasks:
		if event.task.get_instance_id() == task.get_instance_id(): continue
		task.update(event)

func add_task(task) -> void:
	tasks.push_back(task)