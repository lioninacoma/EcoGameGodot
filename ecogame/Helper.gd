extends Node

func flattenIndex(var x, var y, var z):
	return x + WorldVariables.chunkSizeX * (y + WorldVariables.chunkSizeY * z);
