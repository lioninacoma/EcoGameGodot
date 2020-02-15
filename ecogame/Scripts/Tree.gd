extends Spatial

const COLLAPSE_TIMEOUT = 0.5
const DELETE_TIMEOUT = 2.5

var asset
var volume : Array
var time = 0
var collapsed = false
var deleted = false
var voxel_assets = []

func _init(asset, volume : Array) -> void:
	self.asset = asset
	self.volume = volume
	add_child(asset)

func get_body():
	return asset.get_node("body")

func get_mesh():
	return get_body().get_node("mesh")

func _process(delta : float) -> void:
	time += delta
	
	if time > DELETE_TIMEOUT:
		for i in range(voxel_assets.size() - 1, -1, -1):
			var voxel_asset = voxel_assets[i]
			if voxel_asset[0] == 4:
				var voxel_asset_body = voxel_asset[1].get_node("body")
				var velocity_len = voxel_asset_body.get_linear_velocity().length()
				if velocity_len > 0.05: continue
				var vx = int(voxel_asset_body.global_transform.origin.x)
				var vy = int(voxel_asset_body.global_transform.origin.y)
				var vz = int(voxel_asset_body.global_transform.origin.z)
				Lib.instance.setVoxel(Vector3(vx, vy, vz), 4)
				voxel_asset[1].queue_free()
				voxel_assets.remove(i)
			else:
				voxel_asset[1].queue_free()
				voxel_assets.remove(i)
	
	if !collapsed && time > COLLAPSE_TIMEOUT:
		collapsed = true
		for voxel in volume:
			var voxel_pos = voxel.getPosition()
			var meshes = Lib.instance.buildVoxelAssetByVolume([voxel])
			if meshes.size() <= 0: continue

			var voxel_asset = Lib.game.build_asset(meshes, null)
			add_child(voxel_asset)
			
			voxel_pos = voxel_asset.to_local(voxel_pos)
			voxel_asset.transform.origin = voxel_pos + Vector3(0.5, 0.5, 0.5);
			voxel_assets.push_back([voxel.getType(), voxel_asset])
		
		var delta_pos : Vector3 = get_body().global_transform.origin - global_transform.origin
		translate(delta_pos)
		
		var r : Vector3 = get_body().get_rotation()
		rotate(Vector3(1, 0, 0), r.x)
		rotate(Vector3(0, 1, 0), r.y)
		rotate(Vector3(0, 0, 1), r.z)
		asset.queue_free()
