extends KinematicBody
class_name Actor

var TaskHandler = load("res://Scripts/Actor/Task/TaskHandler.gd")
var TaskSeries  = load("res://Scripts/Actor/Task/TaskSeries.gd")

var MoveToTask     = load("res://Scripts/Actor/Task/Misc/MoveToTask.gd")
var FollowPathTask = load("res://Scripts/Actor/Task/Misc/FollowPathTask.gd")

var velocity = Vector3()
var acceleration = Vector3()

# walk variables
var gravity = Vector3(0, -9.8, 0)
const MAX_SPEED = 6
const ACCEL = 2

var task_handler

func get_class():
	return "Actor"

func init(task_manager):
	task_handler = TaskHandler.new()
	task_handler.init(self, task_manager)

func _process(delta : float) -> void:
	update(delta)
	task_handler.perform_tasks(delta)

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
	
	var task_series = TaskSeries.new()
	var follow_path = FollowPathTask.new()
	var move_to = MoveToTask.new()
	
	follow_path.set_path(path)
	move_to.set_to(start)
	task_series.add_task(follow_path)
	task_series.add_task(move_to)
	task_series.set_loop(true)
	task_series.task_name = "GatherWood"
	task_handler.add_task(task_series, false)

func move_to(end : Vector3, interrupt : bool) -> void:
	var move_to = MoveToTask.new()
	move_to.set_to(end)
	task_handler.add_task(move_to, false)
	if interrupt:
		move_to.notify("Interrupt", ["sync"])
