extends Task

var tasks : Array = []
var loop : bool = false

func set_loop(loop : bool):
	self.loop = loop

func _init():
	self.init("TaskSeries")

func perform(delta : float, actor : Actor) -> bool:
	if !tasks.empty():
		var task = tasks[0]
		if task.perform(delta, actor):
			tasks.pop_front()
			if loop: tasks.push_back(task)
	
	return tasks.empty()

func update(event : TaskEvent) -> void:
	for task in tasks:
		if event.task.get_instance_id() == task.get_instance_id(): continue
		task.update(event)

func add_task(task) -> void:
	tasks.push_back(task)