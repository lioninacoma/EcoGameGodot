extends Condition

func condition(actor, context, global_context) -> bool:
	return context.get("path") != null
