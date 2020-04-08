extends KinematicBody
class_name Actor

var world
var velocity = Vector3()
var acceleration = Vector3()

# walk variables
var gravity = Vector3(0, -9.8, 0)
const MAX_SPEED = 6
const ACCEL = 2

onready var context = $BehaviourContext

func _ready():
#	context.set("group", 0)
	pass

func _process(delta : float) -> void:
	update(delta)

func set_world(world):
	self.world = world

func get_world():
	return world

func apply_force(force : Vector3):
	acceleration += force

func update(delta : float) -> void:
	velocity += acceleration
	if velocity.length() >= MAX_SPEED:
		velocity = velocity.normalized()
		velocity *= MAX_SPEED
	move_and_slide(velocity, Vector3(0, 1, 0))
	acceleration *= 0

func balance(g : Vector3):
	var basis = get_global_transform().basis
	var at = basis.z.cross(g)
	var pos = global_transform.origin
	look_at_from_position(Vector3(0, 0, 0), at, basis.y)
	global_transform.origin = pos
