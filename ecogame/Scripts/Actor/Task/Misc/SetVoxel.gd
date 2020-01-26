extends Task

var position
var position_key = null
var voxel
var voxel_key = null

func set_position(position : Vector3):
	self.position = position

func set_position_key(position_key : String):
	self.position_key = position_key

func set_voxel(voxel : int):
	self.voxel = voxel

func set_voxel_key(voxel_key : String):
	self.voxel_key = voxel_key

func _init():
	self.init("SetVoxel")

func perform(delta : float, actor : Actor) -> bool:
	if position_key:
		position = self.get_task_data(position_key)
	
	if voxel_key:
		voxel = self.get_task_data(voxel_key)
	
	if position && voxel:
		Lib.instance.setVoxel(position, voxel)
	return true
