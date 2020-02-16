extends Condition

func condition(actor, context) -> bool:
	return context.get("next_voxel_location") != null
