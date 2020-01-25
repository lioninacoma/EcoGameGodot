extends Task

var path = null
var waypoint = null
var to : Vector3 = Vector3()
var i = 0

func set_to(to : Vector3):
	self.to = to

func _init():
	self.init("MoveTo")

func move_to_waypoint(actor):
	if waypoint == null: return
	var direction = waypoint - actor.global_transform.origin
	direction = direction.normalized()
	actor.velocity = direction * actor.MAX_SPEED

func perform(delta : float, actor : Actor) -> bool:
	if path == null:
		var from = actor.global_transform.origin
		path = Lib.instance.navigate(from, to)
		if path.size() == 0: return true
		waypoint = path[i]
	
	move_to_waypoint(actor)
	
	if waypoint != null && actor.global_transform.origin.distance_to(waypoint) <= 0.4:
		if path.size() > i + 1:
			i += 1
			waypoint = path[i]
		else:
			actor.velocity *= 0
			actor.acceleration *= 0
			i = 0
			waypoint = path[i]
			return true
	return false
