extends Task

var condition_tasks : Array = []
var executing_task = null
var else_task = null

func interrupt():
	.interrupt()
	for task in condition_tasks:
		task[1].interrupt()
	if else_task != null:
		else_task.interrupt()

func finish():
	for task in condition_tasks:
		task[1].finish()
	if else_task != null:
		else_task.finish()

func get_task(task_name: String):
	var t = .get_task(task_name)
	if t != null:
		return t
	
	for task in condition_tasks:
		t = task[1].get_task(task_name)
		if t != null:
			return t
	
	if else_task:
		t = else_task.get_task(task_name)
		if t != null:
			return t
	return null

func add_task(task : Task, condition : FuncRef = null):
	if condition == null:
		else_task = task
	else:
		condition_tasks.push_back([condition, task])

func _init():
	self.init("TaskCondition")

func perform(delta : float, actor : Actor) -> bool:
	if !executing_task:
		for task in condition_tasks:
			if task[0].call_func(actor):
				executing_task = task[1]
				break
	
	if executing_task == null && else_task != null:
		executing_task = else_task
	
	if !executing_task || executing_task.interrupted || executing_task.perform(delta, actor):
		executing_task = null
		return true
	
	return false

func update(event : TaskEvent) -> void:
	.update(event)
	if executing_task != null && event.task != executing_task.get_instance_id():
		if event.receiver_task_name:
			executing_task = executing_task.get_task(event.receiver_task_name)
		if executing_task:
			executing_task.update(event)
	
	if else_task != null && event.task != else_task.get_instance_id():
		if event.receiver_task_name:
			else_task = else_task.get_task(event.receiver_task_name)
		if else_task:
			else_task.update(event)
		
