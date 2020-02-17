extends Behaviour
class_name Sequence

var current_behaviour = null

func reset(context):
	.reset(context)
	for behaviour in get_children():
		if behaviour is Behaviour:
			behaviour.reset(context)

func run(actor, context, global_context) -> bool:
	if current_behaviour != null:
		var finished = current_behaviour.get_state() == STATE.FINISHED
		var stopped = current_behaviour.get_state() == STATE.STOPPED
		if finished || stopped:
			current_behaviour = null
	
	if current_behaviour == null:
		for behaviour in get_children():
			if behaviour is Behaviour && behaviour.state == STATE.READY:
				current_behaviour = behaviour
				break
	
	update_behaviour(current_behaviour, actor, context)
	return current_behaviour != null
