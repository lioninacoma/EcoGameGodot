extends KinematicBody
class_name Actor

var TaskHandler = load("res://Scripts/Actor/Task/TaskHandler.gd")
var TestTask = load("res://Scripts/Actor/Task/TestTask.gd")
var ObserverTask = load("res://Scripts/Actor/Task/ObserverTask.gd")
var MoveToTask = load("res://Scripts/Actor/Task/MoveToTask.gd")
var FollowPathTask = load("res://Scripts/Actor/Task/FollowPathTask.gd")
var TaskSeries = load("res://Scripts/Actor/Task/TaskSeries.gd")

var velocity = Vector3()
var acceleration = Vector3()

# walk variables
var gravity = Vector3(0, -9.8, 0)
const MAX_SPEED = 6
const ACCEL = 2

var taskHandler

func get_class():
	return "Actor"

func init(taskManager):
	taskHandler = TaskHandler.new()
	taskHandler.init(self, taskManager)

func _process(delta : float) -> void:
	update(delta)
	taskHandler.perform_tasks(delta)

func apply_force(force : Vector3):
	acceleration += force

func update(delta : float) -> void:
	velocity += acceleration
	if velocity.length() >= MAX_SPEED:
		velocity = velocity.normalized()
		velocity *= MAX_SPEED
	move_and_slide(velocity, Vector3(0, 1, 0))
	acceleration *= 0

func gather_wood() -> void:
	var start = global_transform.origin
	var path = Lib.instance.navigateToClosestVoxel(start, 4)
	if path.size() == 0: return
	
	var taskSeries = TaskSeries.new()
	var followPath = FollowPathTask.new()
	var moveTo = MoveToTask.new()
	
	followPath.set_path(path)
	moveTo.set_to(start)
	taskSeries.add_task(followPath)
	taskSeries.add_task(moveTo)
	taskSeries.set_loop(true)
	taskHandler.add_task(taskSeries, false)

func move_to(end : Vector3) -> void:
	var moveTo = MoveToTask.new()
	moveTo.set_to(end)
	taskHandler.add_task(moveTo, false)
