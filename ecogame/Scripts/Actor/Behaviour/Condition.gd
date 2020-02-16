extends Sequence
class_name Condition

var condition = null

func reset(context):
	.reset(context)
	condition = null

func condition(actor, context) -> bool:
	return false

func run(actor, context) -> bool:
	if condition == null:
		condition = condition(actor, context)
	if condition:
		return .run(actor, context)
	return false
