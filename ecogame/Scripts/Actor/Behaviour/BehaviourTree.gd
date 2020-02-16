extends Node
class_name BehaviourTree

onready var actor = $"../"
onready var context = $"../BehaviourContext"

var root = null

func _ready():
	root = get_child(0)
	var is_behaviour = root is Behaviour
	if !is_behaviour:
		push_error("root is not a Behaviour")
		get_tree().quit()
		return
	if !actor:
		push_error("actor is empty")
		get_tree().quit()
		return
	if !context:
		push_error("context is empty")
		get_tree().quit()
		return

func _process(delta : float):
	var finished = root.get_state() == Behaviour.STATE.FINISHED
	var stopped = root.get_state() == Behaviour.STATE.STOPPED
	if root && actor && context && root is Behaviour && !finished && !stopped:
		Behaviour.update_behaviour(root, actor, context)
