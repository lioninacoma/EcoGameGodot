extends Node

onready var eco_game = get_tree().get_root().get_node("EcoGame")
onready var controls = $"../"
onready var player = eco_game.get_node("Player")

var camera : Camera
var camera_u : Camera
var drag_enabled = false
var mouse_before = null
var radius : float   = WorldVariables.WORLD_RADIUS
var center : Vector3 = Vector3(radius, radius, radius)
var radius_initial : float
var camera_offset_y : float = 0 

func _ready():
	camera = controls.get_camera()
	camera.look_at(center, Vector3.UP)
	
	camera_u = controls.get_camera_u()
	camera_u.look_at(center, Vector3.UP)
	
	player.global_transform.origin = Vector3(0, radius, 0)
	radius = (player.global_transform.origin - center).length()
	radius_initial = radius

func _process(delta : float) -> void:
	if !drag_enabled:
		mouse_before = null
		return
	
	var mouse_pos_2d = get_viewport().get_mouse_position()
	camera_u.look_at(center, Vector3.UP)
	
	var dropPlane : Plane = camera_u.get_frustum()[1]
	var mouse_pos = dropPlane.intersects_ray(
		camera_u.project_ray_origin(mouse_pos_2d),
		camera_u.project_ray_normal(mouse_pos_2d))
	
	if !mouse_before:
		mouse_before = mouse_pos
		return
	
	if !mouse_pos || !mouse_before:
		return
	
	var delta_mouse : Vector3 = mouse_before - mouse_pos
	player.global_transform.origin += delta_mouse
	self.update_camera()

func update_camera():
	var s = center - player.global_transform.origin
	var t = s.length() - radius
	player.global_transform.origin += s.normalized() * t
	camera.look_at(center + Vector3(0, camera_offset_y, 0), Vector3.UP)

func _input(event : InputEvent) -> void:
	if event is InputEventMouseButton:
		if event.button_index == BUTTON_RIGHT:
			drag_enabled = event.pressed
		elif event.button_index == BUTTON_WHEEL_UP && event.pressed:
			radius -= 1.0
			if radius <= radius_initial:
				camera_offset_y += 3.0
			self.update_camera()
		elif event.button_index == BUTTON_WHEEL_DOWN && event.pressed:
			radius += 1.0
			camera_offset_y -= 3.0
			camera_offset_y = max(camera_offset_y, 0)
			self.update_camera()
