extends Node

var Actor : PackedScene = load("res://Actor.tscn")

var amount_actors : int = 0
var actors : Array = []

# Called when the node enters the scene tree for the first time.
func _ready():
	pass # Replace with function body.

func create_actor(world):
	var actor = Actor.instance()
	actor.set_voxel_world(world)
	world.add_child(actor)
	amount_actors += 1
	actors.push_back(actor)
	return actor

func get_actors():
	return actors

func amount_actors() -> int:
	return amount_actors
