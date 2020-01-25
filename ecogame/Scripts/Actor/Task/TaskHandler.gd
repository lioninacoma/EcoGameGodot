extends Node

var actor
var taskManager
var tasks : Array = []
var tasksAsync : Array = []
var bin : Array = []

func init(actor, taskManager):
	self.actor = actor
	self.taskManager = taskManager
	taskManager.add_handler(self)

func add_task(task, asynch : bool) -> void:
	if asynch:
		tasksAsync.push_back(task)
	else:
		tasks.push_back(task)
	task.set_handler(self)

func perform_tasks(delta : float) -> void:
	var i = 0
	for task in tasksAsync:
		if task.perform(delta, actor):
			bin.push_back(i)
		i += 1
	while !bin.empty():
		i = bin[0]
		bin.pop_front()
		tasksAsync.erase(i)
	
	if !tasks.empty():
		var task = tasks[0]
		if task.perform(delta, actor):
			tasks.pop_front()

func notify(event : TaskEvent, global : bool) -> void:
	if (global):
		taskManager.notify(event)
	update(event)

func update(event) -> void:
	if event.receiver && event.receiver != actor.get_instance_id(): return
	if event.receiver_class && event.receiver_class != actor.get_class(): return
	for task in tasks:
		if event.task.get_instance_id() == task.get_instance_id(): continue
		task.update(event)
	for task in tasksAsync:
		if event.task.get_instance_id() == task.get_instance_id(): continue
		task.update(event)