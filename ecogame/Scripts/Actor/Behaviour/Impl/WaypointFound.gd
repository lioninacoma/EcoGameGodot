extends Condition

func condition(actor, context, global_context) -> bool:
	return global_context.get("waypoint") != null
