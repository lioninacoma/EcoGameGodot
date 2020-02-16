extends Condition

func condition(actor, context) -> bool:
	return context.get("path") != null
