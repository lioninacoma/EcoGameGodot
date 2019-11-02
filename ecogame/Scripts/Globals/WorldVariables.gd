extends Node

const MAX_INT = 9223372036854775807

# world building variables
const WORLD_SIZE = 16;
const CHUNK_SIZE_X = 16
const CHUNK_SIZE_Y = 256
const CHUNK_SIZE_Z = 16
const PICK_DISTANCE = 50
const NOISE_SCALE = 1.5

var noiseSeed = randi()
var terrainMaterial = SpatialMaterial.new()
var terrainTexture = preload("res://Images/grass.png")

func _ready():
	terrainMaterial.albedo_texture = terrainTexture
