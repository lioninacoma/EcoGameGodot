extends Node
class_name Task

var taskName : String
var taskHandler

func init(taskName : String):
	self.taskName = taskName

func set_handler(taskHandler) -> void:
	self.taskHandler = taskHandler

func notify(args : Array, config : Dictionary):
	var global = config["global"] if config.has("global") else false
	var event = TaskEvent.new()
	event.handler = taskHandler
	event.task = self
	event.args = args
	event.receiver = config["receiver"] if config.has("receiver") else null
	event.receiver_class = config["receiver_class"] if config.has("receiver_class") else null
	taskHandler.notify(event, global)

# Child class implements this method
func perform(delta : float, actor : Actor) -> bool:
	return true

# Child class implements this method
func update(event : TaskEvent) -> void:
	pass