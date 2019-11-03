extends KinematicBody

var camera_angle : float = 0.0
var mouse_sensitivity : float = 0.3

var velocity : Vector3 = Vector3()
var direction : Vector3 = Vector3()

# fly variables
const FLY_SPEED : float = 40.0
const FLY_ACCEL : float = 4.0

# walk variables
#const GRAVITY = -9.8 * 3
const GRAVITY : float = 0.0
const MAX_SPEED : float = 100.0
const MAX_RUNNING_SPEED : float = 130.0
const ACCEL : float = 2.0
const DEACCEL : float = 6.0

# jumping
const JUMP_HEIGHT : float = 50.0

# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	pass # Replace with function body.

func _physics_process(delta : float) -> void:
	move(delta)

func _input(event : InputEvent) -> void:
	if event is InputEventMouseMotion:
		$Head.rotate_y(deg2rad(-event.relative.x * mouse_sensitivity))
		
		var change = -event.relative.y * mouse_sensitivity
		if change + camera_angle < 90 and change + camera_angle > -90:
			$Head/Camera.rotate_x(deg2rad(change))
			camera_angle += change

func move(delta : float) -> void:
	# reset direction of the player
	direction = Vector3()
	
	# get rotation of the camera
	var aim = $Head/Camera.get_global_transform().basis
	if Input.is_action_pressed("ui_up"):
		direction -= aim.z
	if Input.is_action_pressed("ui_down"):
		direction += aim.z
	if Input.is_action_pressed("ui_left"):
		direction -= aim.x
	if Input.is_action_pressed("ui_right"):
		direction += aim.x
	
	direction = direction.normalized()
	
	velocity.y += GRAVITY * delta
	
	var temp_velocity = velocity
	temp_velocity.y = 0
	
	var speed = MAX_SPEED
#	if Input.is_action_pressed("move_sprint"):
#		speed = MAX_RUNNING_SPEED
#	else:
#		speed = MAX_SPEED
	
	# where would the player go at max speed
	var target = direction * speed
	
	var acceleration
	if direction.dot(temp_velocity) > 0:
		acceleration = ACCEL
	else:
		acceleration = DEACCEL
	
	# calculate a portion of the distance to go
	temp_velocity = temp_velocity.linear_interpolate(target, acceleration * delta)
	
	velocity.x = temp_velocity.x
	velocity.z = temp_velocity.z
	
	# move
	velocity = move_and_slide(velocity, Vector3(0, 1, 0))
	
	if Input.is_action_pressed("jump"):
		velocity.y = JUMP_HEIGHT
	elif Input.is_action_pressed("move_sprint"):
		velocity.y = -JUMP_HEIGHT
	else:
		velocity.y = 0