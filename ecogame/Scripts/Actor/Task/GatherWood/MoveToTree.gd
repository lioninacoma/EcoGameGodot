extends TaskSeries

var FindTreeTask   = load("res://Scripts/Actor/Task/GatherWood/FindTree.gd")
var FollowPathTask = load("res://Scripts/Actor/Task/Misc/FollowPath.gd")

func _init():
	self.init("MoveToTree")
	var find_tree = FindTreeTask.new()
	var follow_path = FollowPathTask.new()
	follow_path.set_path_key("path_to_tree")
	self.add_task(find_tree)
	self.add_task(follow_path)