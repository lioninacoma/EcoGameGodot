extends Node
class_name Task

var task_name : String
var interrupted = false
var is_async = false

var event_listeners : Dictionary = {}

func interrupt():
	self.interrupted = true

func on_interrupt(event : TaskEvent, context):
	if event.args.empty() || is_async == (event.args[0] == "async"):
		interrupt()

func init(task_name : String):
	self.task_name = task_name
	add_event_listener("Interrupt", funcref(self, "on_interrupt"))

func notify(actor, event_name : String, args : Array = [], config : Dictionary = {}):
	var global = config["global"] if config.has("global") else false
	var event = TaskEvent.new()
	event.event_name = event_name
	event.args = args
	event.handler = actor.task_handler
	event.task = self
	event.exclude_self = config["exclude_self"] if config.has("exclude_self") else false
	event.receiver = config["receiver"] if config.has("receiver") else null
	event.receiver_task_name = config["receiver_task_name"] if config.has("receiver_task_name") else null
	event.receiver_class = config["receiver_class"] if config.has("receiver_class") else null
	event.handler.notify(event, global)

# Child class implements this method
func perform(delta : float, actor) -> bool:
	return true

func update(event : TaskEvent) -> void:
	for f in event_listeners[event.event_name]:
		f.call_func(event, self)

func add_event_listener(event_name : String, f : FuncRef):
	if !event_listeners.has(event_name):
		event_listeners[event_name] = []
	event_listeners[event_name].push_back(f)