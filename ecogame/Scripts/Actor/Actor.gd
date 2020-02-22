extends KinematicBody
class_name Actor

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

func apply_force(force : Vector3):
	acceleration += force

func update(delta : float) -> void:
	velocity += acceleration
	if velocity.length() >= MAX_SPEED:
		velocity = velocity.normalized()
		velocity *= MAX_SPEED
	move_and_slide(velocity, Vector3(0, 1, 0))
	acceleration *= 0
