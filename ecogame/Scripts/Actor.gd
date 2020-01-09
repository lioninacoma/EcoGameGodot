extends KinematicBody
class_name Actor

var path
var waypoint

var velocity = Vector3()
var acceleration = Vector3()

# walk variables
var gravity = Vector3(0, -9.8, 0)
const MAX_SPEED = 6
const ACCEL = 2

func _ready():
	pass

func _process(delta : float) -> void:
	var friction = velocity
	friction *= -1
	friction = friction.normalized()
	friction *= 0.1
	
#	self.applyForce(friction)
#	self.applyForce(gravity)
	
	self.move_to_waypoint()
	self.update()
	

func applyForce(force : Vector3):
	acceleration += force

func update() -> void:
	velocity += acceleration
	if velocity.length() >= MAX_SPEED:
		velocity = velocity.normalized()
		velocity *= MAX_SPEED
	move_and_slide(velocity, Vector3(0, 1, 0))
	acceleration *= 0

func move_to_waypoint():
	if !waypoint:
		return
	var direction = waypoint - global_transform.origin
	direction = direction.normalized()
	velocity = direction * MAX_SPEED
	if global_transform.origin.distance_to(waypoint) <= 0.6:
		if path.size() > 0:
			waypoint = path[0]
			path.remove(0)
		else:
			velocity *= 0
			acceleration *= 0
			path = null
			waypoint = null

func follow_path(path : PoolVector3Array) -> void:
	if path.size() == 0:
		return
	self.path = path
	waypoint = path[0]
	path.remove(0)
