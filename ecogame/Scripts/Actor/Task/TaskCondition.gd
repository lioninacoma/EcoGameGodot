extends Task

var condition_tasks : Array = []
var executing_task = null

func add_task(condition : FuncRef, task : Task):
	condition_tasks.push_back([condition, task])

func _init():
	self.init("TaskCondition")

func perform(delta : float, actor : Actor) -> bool:
	if !executing_task:
		for task in condition_tasks:
			if task[0].call_func(actor, task_handler.task_data):
				executing_task = task[1]
				break
	
	if !executing_task || executing_task.interrupted || executing_task.perform(delta, actor):
		return true
	
	return false

func update(event : TaskEvent) -> void:
	.update(event)
	if executing_task:
		executing_task.update(event)