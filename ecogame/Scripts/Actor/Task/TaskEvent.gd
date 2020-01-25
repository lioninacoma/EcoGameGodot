extends Node
class_name TaskEvent

# meta
var event_name
var handler
var task

# filter
var exclude_self = false
var receiver = null
var receiver_class = null
var receiver_task_name = null

# arguments
var args : Array = []