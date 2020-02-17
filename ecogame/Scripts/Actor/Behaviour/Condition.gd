extends Sequence
class_name Condition

var condition = null
var else_case = null

func _ready():
	._ready()
	else_case = get_node_or_null("Else")

func reset(context):
	.reset(context)
	condition = null

func condition(actor, context, global_context) -> bool:
	return false

func run(actor, context, global_context) -> bool:
	if condition == null:
		condition = condition(actor, context, global_context)
	if condition:
		if else_case != null:
			else_case.end()
		return .run(actor, context, global_context)
	elif else_case != null:
		return else_case.run(actor, context, global_context)
	return false
