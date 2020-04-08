extends Node

onready var eco_game = get_tree().get_root().get_node("EcoGame")
onready var controls = $"../"

var angle
var length
var steps
var step
var axiom
var sentence
var rules = [];
var state_cache = []
var current_position
var current_rotation = Vector2()
var noise = OpenSimplexNoise.new()

# Called when the node enters the scene tree for the first time.
func _ready():
	noise.set_seed(123)
	noise.set_octaves(6)
	noise.set_period(192.0)
	noise.set_persistence(0.5)
	angle = deg2rad(25.7)
	length = 1.5
	steps = 25
	step = length / steps
	axiom = "A"
	sentence = axiom
	rules.push_back({
	  "a": "A",
	  "b": "B[+A]B[-A]B[*A]B[/A]+A"
	})
	rules.push_back({
	  "a": "B",
	  "b": "BB"
	})
	for i in range(3):
		generate()

func generate():
	var nextSentence = "";
	for current in sentence:
		var found = false
		for r in rules:
			if current == r["a"]:
				found = true
				nextSentence += r["b"]
				break;
		if !found:
			nextSentence += current
	sentence = nextSentence;

func build_tree(pos : Vector3, world):
	current_position = pos
	current_rotation = Vector2()
	state_cache.clear()
	
	for current in sentence:
#		var current_noise = noise.get_noise_3dv(current_position)
#		if current_noise <= -0.2:
#			pass
#		elif current_noise > -0.2: 
#			current_rotation.x += angle
#		elif current_noise > 0.0: 
#			current_rotation.y += angle
#		elif current_noise > 0.2: 
#			current = "]"
#		elif current_noise > 0.4:
#			pass
		if current == "A" || current == "B" || current == "C":
			var delta = Vector3(0, 1, 0)
			delta = delta.rotated(Vector3(1, 0, 0), current_rotation.x)
			delta = delta.rotated(Vector3(0, 0, 1), current_rotation.y)
			delta = delta.normalized() * length
			var toward = current_position + delta;
			for s in range(steps):
				current_position = current_position.move_toward(toward, step)
				world.setVoxel(current_position, 1.2, true)
			current_position = toward
		elif current == "+":
			current_rotation.x += angle
		elif current == "-":
			current_rotation.x -= angle
		elif current == "*":
			current_rotation.y += angle
		elif current == "/":
			current_rotation.y -= angle
		elif current == "[":
			state_cache.push_back({
				"pos": current_position,
				"rot": current_rotation
			})
		elif current == "]":
			var current_state = state_cache.pop_back()
			if current_state != null:
				current_position = current_state["pos"]
				current_rotation = current_state["rot"]

func _input(event : InputEvent) -> void:
	if event is InputEventMouseButton:
		if event.button_index == BUTTON_LEFT and event.pressed and controls.control_active:
			var from = controls.camera.project_ray_origin(event.position)
			var to = from + controls.camera.project_ray_normal(event.position) * WorldVariables.PICK_DISTANCE
			var space_state = eco_game.get_world().direct_space_state
			var result = space_state.intersect_ray(from, to)
		
			if result:
				var voxel_position : Vector3 = controls.get_voxel_position(result)
				var world = result.collider.shape_owner_get_owner(0)
#				var n = (noise.get_noise_3dv(voxel_position * 8) + 1.0) / 2.0
#				n += 0.25
#				angle = deg2rad(35.7) * n
#				length = 2.0 * n
#				print ("%s/%s"%[angle, length])
				build_tree(voxel_position, world)
				
