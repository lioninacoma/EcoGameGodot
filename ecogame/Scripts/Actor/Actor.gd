extends KinematicBody
class_name Actor

var TaskHandler   = load("res://Scripts/Actor/Task/TaskHandler.gd")
var TaskSeries    = load("res://Scripts/Actor/Task/TaskSeries.gd")
var TaskCondition = load("res://Scripts/Actor/Task/TaskCondition.gd")

var MoveToTask     = load("res://Scripts/Actor/Task/Misc/MoveTo.gd")
var FollowPathTask = load("res://Scripts/Actor/Task/Misc/FollowPath.gd")

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

func cond_a(actor, data):
	return true

func cond_b(actor, data):
	return false

func condition_test():
	var cond_a_fn = funcref(self, "cond_a")
	var cond_b_fn = funcref(self, "cond_b")
	var move_to_a = MoveToTask.new()
	var move_to_b = MoveToTask.new()
	var condition = TaskCondition.new()
	
	move_to_a.set_to(Vector3(0, 0, 0))
	move_to_b.set_to(Vector3(128, 0, 128))
	
	condition.add_task(cond_a_fn, move_to_a)
	condition.add_task(cond_b_fn, move_to_b)
	
	task_handler.add_task(condition, false)

func gather_wood(storehouse_location : Vector3) -> void:
	var path_to_tree_initial = Lib.instance.navigateToClosestVoxel(global_transform.origin, 4)
	if path_to_tree_initial.size() == 0: return
	var path_from_storehouse_to_tree = Lib.instance.navigate(storehouse_location, path_to_tree_initial[path_to_tree_initial.size() - 1])
	if path_from_storehouse_to_tree.size() == 0: return
	
	var gather_wood = TaskSeries.new()
	var follow_path_to_tree_initial = FollowPathTask.new()
	var follow_path_to_tree = FollowPathTask.new()
	var move_to_storehouse = MoveToTask.new()
	
	follow_path_to_tree_initial.set_path(path_to_tree_initial)
	follow_path_to_tree.set_path(path_from_storehouse_to_tree)
	move_to_storehouse.set_to(storehouse_location)
	
	gather_wood.add_task(move_to_storehouse)
	gather_wood.add_task(follow_path_to_tree)
	gather_wood.set_loop(true)
	gather_wood.task_name = "GatherWood"
	
	task_handler.add_task(follow_path_to_tree_initial, false)
	task_handler.add_task(gather_wood, false)

func move_to(end : Vector3, interrupt : bool) -> void:
	var move_to = MoveToTask.new()
	move_to.set_to(end)
	task_handler.add_task(move_to, false)
	if interrupt:
		move_to.notify("Interrupt", ["sync"])
