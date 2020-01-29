extends Task
class_name TaskSeries

var tasks : Array = []
var tasks_async : Array = []
var loop : bool = false
var i = 0

func interrupt():
	.interrupt()
	for task in tasks:
		task.interrupt()
	for task in tasks_async:
		task.interrupt()
	loop = false

func finish():
	for task in tasks:
		task.finish()
	for task in tasks_async:
		task.finish()

func set_loop(loop : bool):
	self.loop = loop

func _init():
	self.init("TaskSeries")

func get_task(task_name: String):
	var t = .get_task(task_name)
	if t != null:
		return t
	
	for task in tasks:
		t = task.get_task(task_name)
		if t != null:
			return t
	
	for task in tasks_async:
		t = task.get_task(task_name)
		if t != null:
			return t
	return null

func perform(delta : float, actor) -> bool:
	if !tasks.empty():
		var task = tasks[i]
		if task.interrupted || task.perform(delta, actor):
			i += 1
		
		if i == tasks.size():
			i = 0
			return !loop
		
		for task_a in tasks_async:
			if task_a.interrupted || task_a.perform(delta, actor):
				pass
		return false
		
	return true

func update(event : TaskEvent) -> void:
	.update(event)
	for task in tasks:
		if event.task == task.get_instance_id(): continue
		if event.receiver_task_name:
			task = task.get_task(event.receiver_task_name)
			if task == null: continue
		task.update(event)
	for task in tasks_async:
		if event.task == task.get_instance_id(): continue
		if event.receiver_task_name:
			task = task.get_task(event.receiver_task_name)
			if task == null: continue
		task.update(event)

func add_task(task, async : bool = false) -> void:
	if async:
		tasks_async.push_back(task)
		task.is_async = true
	else:
		tasks.push_back(task)