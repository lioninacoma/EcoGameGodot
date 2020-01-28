extends KinematicBody
class_name Actor

var TaskHandler   = load("res://Scripts/Actor/Task/TaskHandler.gd")
var TaskSeries    = load("res://Scripts/Actor/Task/TaskSeries.gd")
var TaskCondition = load("res://Scripts/Actor/Task/TaskCondition.gd")
var TaskEnd       = load("res://Scripts/Actor/Task/TaskEnd.gd")

var FindPathToVoxel = load("res://Scripts/Actor/Task/Base/FindPathToVoxel.gd")
var FindVoxelNearby = load("res://Scripts/Actor/Task/Base/FindVoxelNearby.gd")
var FollowPath      = load("res://Scripts/Actor/Task/Base/FollowPath.gd")
var MoveTo          = load("res://Scripts/Actor/Task/Base/MoveTo.gd")
var SetVoxel        = load("res://Scripts/Actor/Task/Base/SetVoxel.gd")

var velocity = Vector3()
var acceleration = Vector3()

# walk variables
var gravity = Vector3(0, -9.8, 0)
const MAX_SPEED = 6
const ACCEL = 2

onready var EcoGame = get_tree().get_root().get_node("EcoGame")
var task_handler

func _ready():
	task_handler = TaskHandler.new(self, EcoGame.task_manager)

func get_class():
	return "Actor"

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

func path_found(actor):
	var data = actor.task_handler.task_data
	return data.has("path") && data["path"] != null

func voxel_found(actor):
	var data = actor.task_handler.task_data
	return data.has("voxel_nearby") && data["voxel_nearby"] != null

func gather_wood(storehouse_location : Vector3) -> void:
	var gather_wood = TaskSeries.new()
	gather_wood.task_name = "GatherWood"
	gather_wood.set_loop(true)
	
	var find_path_to_tree = FindPathToVoxel.new(4)
	var is_path_found = TaskCondition.new()
	var follow_path_to_tree = FollowPath.new({"path_key": "path"})
	var task_end = TaskEnd.new(gather_wood.task_name)
	
	is_path_found.add_task(follow_path_to_tree, funcref(self, "path_found"))
	is_path_found.add_task(task_end) # ELSE
	
	var find_trunk_nearby = FindVoxelNearby.new({"voxel": 4})
	var is_voxel_found = TaskCondition.new()
	var chop_wood_and_store = TaskSeries.new()
	var chop_wood = SetVoxel.new({"position_key": "voxel_nearby", "voxel": 0})
	var move_to_storehouse = MoveTo.new(storehouse_location)
	
	chop_wood_and_store.add_task(chop_wood)
	chop_wood_and_store.add_task(move_to_storehouse)
	is_voxel_found.add_task(chop_wood_and_store, funcref(self, "voxel_found"))
	
	gather_wood.add_task(find_path_to_tree)
	gather_wood.add_task(is_path_found)
	gather_wood.add_task(find_trunk_nearby)
	gather_wood.add_task(is_voxel_found)
	
	task_handler.add_task(gather_wood, false)

func move_to(end : Vector3, interrupt : bool) -> void:
	var move_to = MoveTo.new(end)
	task_handler.add_task(move_to, false)
	if interrupt:
		move_to.notify("Interrupt", ["sync"])
