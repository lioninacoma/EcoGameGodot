extends Node

# world building variables
const WORLD_SIZE : int = 64;
const CHUNK_SIZE_X : int = 16
const CHUNK_SIZE_Y : int = 256
const CHUNK_SIZE_Z : int = 16
const PICK_DISTANCE : int = 10
const BUILD_DISTANCE : int = 128
const NOISE_SCALE : float = 1.5
const NOISE_SEED : int = 512365

var materials : Array = []
var grassMaterial : SpatialMaterial = SpatialMaterial.new()
var stoneMaterial : SpatialMaterial = SpatialMaterial.new()
var dirtMaterial : SpatialMaterial = SpatialMaterial.new()
var woodMaterial : SpatialMaterial = SpatialMaterial.new()
var leavesMaterial : SpatialMaterial = SpatialMaterial.new()

var grassTexture : Texture = preload("res://Images/grass.png")
var stoneTexture : Texture = preload("res://Images/stone.png")
var dirtTexture : Texture = preload("res://Images/dirt.png")
var woodTexture : Texture = preload("res://Images/wood.png")
var leavesTexture : Texture = preload("res://Images/leaves.png")

func _ready() -> void:
	grassMaterial.albedo_texture = grassTexture
	stoneMaterial.albedo_texture = stoneTexture
	dirtMaterial.albedo_texture = dirtTexture
	woodMaterial.albedo_texture = woodTexture
	leavesMaterial.albedo_texture = leavesTexture
	
	materials.push_back(grassMaterial)
	materials.push_back(stoneMaterial)
	materials.push_back(dirtMaterial)
	materials.push_back(woodMaterial)
	materials.push_back(leavesMaterial)
