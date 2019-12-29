extends KinematicBody

var camera_angle_y = 0
var camera_angle_x = 0
var mouse_sensitivity = 0.3

var velocity = Vector3()
var direction = Vector3()
var zoom_pos = Vector2()

# fly variables
const FLY_SPEED = 40
const FLY_ACCEL = 4

# walk variables
var gravity = -9.8 * 3
#var gravity = 0.0
const MAX_SPEED = 12
const MAX_RUNNING_SPEED = 20
const ACCEL = 2
const DEACCEL = 6

#rts variables
const MOVE_MARGIN = 20
const MOVE_SPEED = 30
const ZOOM_SPEED = 120

# jumping
var jump_height = 30
var is_fly = true
var is_rts = false

# Called when the node enters the scene tree for the first time.
func _ready():
	camera_angle_y = $Head.rotation.y
	pass # Replace with function body.

func _physics_process(delta):
	if Input.is_action_just_pressed("switch_fly"):
		is_fly = !is_fly
		$Capsule.disabled = is_fly
		if (is_fly):
			jump_height = 30
		else:
			jump_height = 10
	
	if Input.is_action_just_pressed("switch_rts"):
		is_rts = !is_rts
	
	if Input.is_action_just_pressed("switch_col"):
		$Capsule.disabled = !$Capsule.disabled
	
	if (is_rts):
		rts(delta)
	elif (is_fly):
		fly(delta)
	else:
		walk(delta)

func _input(event):
	if event is InputEventMouseMotion and !is_rts:
		$Head.rotate_y(deg2rad(-event.relative.x * mouse_sensitivity))
		
		var change_y = event.relative.x * mouse_sensitivity
		var change_x = -event.relative.y * mouse_sensitivity
		
		if change_x + camera_angle_x < 90 and change_x + camera_angle_x > -90:
			$Head/Camera.rotate_x(deg2rad(change_x))
			camera_angle_y += change_y
			camera_angle_x += change_x
	elif event is InputEventMouseButton and is_rts:
		if event.is_pressed():
			direction = Vector3()
			
			# get rotation of the camera
			var aim = $Head/Camera.get_global_transform().basis
			var delta = get_physics_process_delta_time()
			
			# zoom in
			if event.button_index == BUTTON_WHEEL_UP:
				direction -= aim.z
			# zoom out
			if event.button_index == BUTTON_WHEEL_DOWN:
				direction += aim.z
			
			direction = direction.normalized()
			global_translate(direction * delta * ZOOM_SPEED)

func rts(delta):
	var vSize = get_viewport().size
	var mousePos = get_viewport().get_mouse_position()
	
	direction = Vector3()
	
	# get rotation of the camera
	var aim = $Head/Camera.get_global_transform().basis
	if mousePos.y < MOVE_MARGIN || Input.is_action_pressed("ui_up"):
		direction -= aim.z
	if mousePos.y > vSize.y - MOVE_MARGIN || Input.is_action_pressed("ui_down"):
		direction += aim.z
	if mousePos.x < MOVE_MARGIN || Input.is_action_pressed("ui_left"):
		direction -= aim.x
	if mousePos.x > vSize.x - MOVE_MARGIN || Input.is_action_pressed("ui_right"):
		direction += aim.x
	
	direction.y = 0
	direction = direction.normalized()
	
	global_translate(direction * delta * MOVE_SPEED)

func walk(delta):
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
	
	velocity.y += gravity * delta
	
	var temp_velocity = velocity
	temp_velocity.y = 0
	
	var speed
	if Input.is_action_pressed("move_sprint"):
		speed = MAX_RUNNING_SPEED
	else:
		speed = MAX_SPEED
	
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
	
	if Input.is_action_just_pressed("jump"):
		velocity.y = jump_height

func fly(delta):
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
	
	# where would the player go at max speed
	var target = direction * FLY_SPEED
	
	# calculate a portion of the distance to go
	velocity = velocity.linear_interpolate(target, FLY_ACCEL * delta)
	
	# move
	move_and_slide(velocity)
	
	if Input.is_action_pressed("jump"):
		velocity.y = jump_height
	elif Input.is_action_pressed("move_sprint"):
		velocity.y = -jump_height