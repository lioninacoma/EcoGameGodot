extends Node

var _pool : ObjectPool

func _init():
	self._pool = ObjectPool.new(8, "res://bin/ecogame.gdns")

func borrow():
	return _pool.borrow_object()

func ret(object):
	_pool.return_object(object)