extends Node

# world building variables
const WORLD_SIZE : int = 32;
const CHUNK_SIZE_X : int = 16
const CHUNK_SIZE_Y : int = 256
const CHUNK_SIZE_Z : int = 16
const PICK_DISTANCE : int = 10
const BUILD_DISTANCE : int = 1200
const NOISE_SCALE : float = 1.5
const NOISE_SEED : int = 512365

var terrainMaterial : SpatialMaterial = SpatialMaterial.new()
var terrainTexture : Texture = preload("res://Images/grass.png")

func _ready() -> void:
	terrainMaterial.albedo_texture = terrainTexture
