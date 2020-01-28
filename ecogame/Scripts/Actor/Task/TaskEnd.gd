extends Task

var receiver_task_name : String

func _init(receiver_task_name : String):
	self.receiver_task_name = receiver_task_name
	self.init("TaskEnd")

func perform(delta : float, actor) -> bool:
	actor.velocity *= 0
	actor.acceleration *= 0
#	print ("_____________TASK END")
	self.notify(actor, "Interrupt", ["sync"], {"receiver_task_name": receiver_task_name})
	return true