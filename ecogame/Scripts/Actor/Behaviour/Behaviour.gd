extends Node
class_name Behaviour

enum STATE {
	READY,
	RUNNING,
	STOPPED,
	FINISHED
}

var state
var repeat

func _ready():
	state = STATE.READY
	repeat = true if get_node_or_null("Repeat") != null else false

func end():
	state = STATE.FINISHED

func stop():
	state = STATE.STOPPED

func reset(context):
	state = STATE.READY

func get_state():
	return state

func run(actor, context, global_context) -> bool:
	return false

static func update_behaviour(behaviour, actor, context):
	if !behaviour: return
	var finished = behaviour.get_state() == STATE.FINISHED
	var stopped = behaviour.get_state() == STATE.STOPPED
	if !finished && !stopped:
		behaviour.state = STATE.RUNNING
		if !behaviour.run(actor, context, BehaviourGlobals.global_context):
			behaviour.end()
			if behaviour.repeat:
				behaviour.reset(context)
