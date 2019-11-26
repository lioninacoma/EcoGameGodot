extends Node

# world building variables
const WORLD_SIZE : int = 32;
const CHUNK_SIZE_X : int = 16
const CHUNK_SIZE_Y : int = 256
const CHUNK_SIZE_Z : int = 16
const PICK_DISTANCE : int = 10
const BUILD_DISTANCE : int = 600
const NOISE_SCALE : float = 1.5
const NOISE_SEED : int = 512365

var materials : Array = []
var grassMaterial : SpatialMaterial = SpatialMaterial.new()
var stoneMaterial : SpatialMaterial = SpatialMaterial.new()

var grassTexture : Texture = preload("res://Images/grass.png")
var stoneTexture : Texture = preload("res://Images/stone.png")

func _ready() -> void:
	grassMaterial.albedo_texture = grassTexture
	stoneMaterial.albedo_texture = stoneTexture
	
	materials.push_back(grassMaterial)
	materials.push_back(stoneMaterial)
