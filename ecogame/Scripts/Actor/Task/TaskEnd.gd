extends Task

var receiver_task_name : String
var finish : bool

func _init(receiver_task_name : String, finish : bool = false):
	self.receiver_task_name = receiver_task_name
	self.finish = finish
	self.init("TaskEnd")

func perform(delta : float, actor) -> bool:
	actor.velocity *= 0
	actor.acceleration *= 0
	var event_name = "Interrupt"
	if finish: event_name = "Finish"
	self.notify(actor, event_name, ["sync"], {"receiver_task_name": receiver_task_name})
#	self.notify(actor, "Interrupt", ["sync"], {"global": true, "exclude_self": true})
#	print ("_____________END TASK %s by %s"%[receiver_task_name, event_name])
	return true
