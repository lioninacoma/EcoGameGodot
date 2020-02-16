extends Condition

func condition(actor, context) -> bool:
	return context.get("voxels_in_area") != null
