extends Condition

func condition(actor, context, global_context) -> bool:
	return context.get("next_voxel_location") != null
