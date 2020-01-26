extends TaskSeries

var FindVoxelNearby = load("res://Scripts/Actor/Task/Misc/FindVoxelNearby.gd")
var SetVoxel        = load("res://Scripts/Actor/Task/Misc/SetVoxel.gd")

var voxel
var voxel_key = null

func set_voxel(voxel : int):
	self.voxel = voxel

func set_voxel_key(voxel_key : String):
	self.voxel_key = voxel_key

func _init():
	self.init("SetVoxelNearby")
	
