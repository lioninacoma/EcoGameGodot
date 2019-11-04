extends Node
class_name ObjectPool

var _pool : Array
var _path : String
var _mutex : Mutex
var _semaphore : Semaphore

func _init(poolSize : int, path : String) -> void:
	self._pool = []
	self._path = path
	self._mutex = Mutex.new()
	self._semaphore = Semaphore.new()
	self._pool.resize(poolSize)
	
	var object = load(path)
	for i in poolSize:
		_pool[i] = object.new()
		_semaphore.post()

func _pop_object():
	for i in _pool.size():
		var object = _pool[i]
		if object:
			_pool[i] = null
			return object
	return null

func _push_object(object):
	for i in _pool.size():
		if !_pool[i]:
			_pool[i] = object

func borrow_object():
	var object = null
	
	while !object:
		_semaphore.wait()
		_mutex.lock()
		object = _pop_object()
		_mutex.unlock()
	
	return object

func return_object(object) -> void:
	if object.get_script().get_path() != _path: 
		return
	_mutex.lock()
	object = _push_object(object)
	_mutex.unlock()
	_semaphore.post()
