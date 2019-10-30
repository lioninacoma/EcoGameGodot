extends Node

# world building variables
var worldSize = 16;
var chunkSizeX = 16
var chunkSizeY = 256
var chunkSizeZ = 16
var noiseScale = 1.5
var terrainMaterial = SpatialMaterial.new()
var terrainTexture = preload("res://Images/grass.png")

func _ready():
	terrainMaterial.albedo_texture = terrainTexture
