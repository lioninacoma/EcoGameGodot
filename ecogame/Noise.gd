extends Node

var noise = OpenSimplexNoise.new()

func _ready():
	noise.seed = randi()
	noise.octaves = 4
	noise.period = 60.0
	noise.persistence = 0.5
