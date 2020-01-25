extends Node

var actor
var task_manager
var tasks : Array = []
var tasks_async : Array = []
var bin : Array = []

func init(actor, task_manager):
	self.actor = actor
	self.task_manager = task_manager
	task_manager.add_handler(self)

func add_task(task, asynch : bool) -> void:
	if asynch:
		tasks_async.push_back(task)
		task.is_async = true
	else:
		tasks.push_back(task)
	task.set_handler(self)

func perform_tasks(delta : float) -> void:
	var i = 0
	for task in tasks_async:
		if task.interrupted || task.perform(delta, actor):
			bin.push_back(i)
		i += 1
	while !bin.empty():
		i = bin[0]
		bin.pop_front()
		tasks_async.erase(i)
	
	if !tasks.empty():
		var task = tasks[0]
		if task.interrupted || task.perform(delta, actor):
			tasks.pop_front()

func notify(event : TaskEvent, global : bool) -> void:
	if (global):
		task_manager.notify(event)
	if (event.exclude_self != null && event.exclude_self): return
	update(event)

func update(event) -> void:
	if event.receiver && event.receiver != actor.get_instance_id(): return
	if event.receiver_class && event.receiver_class != actor.get_class(): return
	for task in tasks:
		if event.task.get_instance_id() == task.get_instance_id(): continue
		if event.receiver_task_name && event.receiver_task_name != task.task_name: continue
		task.update(event)
	for task in tasks_async:
		if event.task.get_instance_id() == task.get_instance_id(): continue
		if event.receiver_task_name != task.task_name: continue
		task.update(event)