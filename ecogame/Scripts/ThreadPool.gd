extends Node
class_name ThreadPool

var _threads : Array
var _taskQueue : Array
var _running : bool
var _mutex : Mutex
var _semaphore : Semaphore

func _init(poolSize : int) -> void:
	self._threads = []
	self._taskQueue = []
	self._running = true
	self._mutex = Mutex.new()
	self._semaphore = Semaphore.new()
	
	_threads.resize(poolSize)
	for i in poolSize:
		_threads[i] = Thread.new()
		_threads[i].start(self, "_task_run")

func shut_down():
	_running = false
	for thread in _threads:
		thread.wait_to_finish()

func submit_task(task : FuncRef, userdata : Array) -> void:
	_mutex.lock()
	_taskQueue.push_back([task, userdata])
	_mutex.unlock()
	_semaphore.post()

func _task_run(data) -> void:
	while (_running):
		var taskdata = null
		_semaphore.wait()
		
		_mutex.lock()
		if !_taskQueue.empty():
			taskdata = _taskQueue.pop_front()
		_mutex.unlock()
		
		if !taskdata || taskdata.empty(): continue
		
		var task : FuncRef = taskdata[0]
		var userdata = taskdata[1]
		
		if !userdata || userdata.empty():
			task.call_func()
		else:
			task.call_func(userdata)
