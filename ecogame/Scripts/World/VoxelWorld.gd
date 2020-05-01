extends WorldObject
class_name VoxelWorld

var VoxelWorldNative = load("res://bin/VoxelWorld.gdns")

# config
var voxel_body
var material : SpatialMaterial

func _init(position : Vector3, chunk_dims : Vector3, material : SpatialMaterial).(position):
	self.material = material
	
	voxel_body = VoxelWorldNative.new()
	voxel_body.setDimensions(Vector2(chunk_dims.x, chunk_dims.z))
	voxel_body.setIsVoxelFn(funcref(self, "is_voxel"))
	voxel_body.setIsWalkableFn(funcref(self, "is_walkable"))
	add_child(voxel_body)
	
	voxel_body.global_translate(-0.5 * WorldVariables.CHUNK_SIZE_X * chunk_dims)
	global_translate(position)

func _ready():
	voxel_body.buildChunks()

func is_voxel(ix : int, iy : int, iz : int, offset : Vector3) -> float:
	return 1.0

func is_walkable(from : Vector3, to : Vector3, normal : Vector3):
	return false

func delete_chunk(chunk, world) -> void:
	var old_mesh_instance_id = chunk.getMeshInstanceId()
	if old_mesh_instance_id != 0:
		var old_mesh_instance = instance_from_id(old_mesh_instance_id)
		if old_mesh_instance:
			world.remove_child(old_mesh_instance)
			old_mesh_instance.free()

func build_chunk(mesh_data : Array, chunk, world) -> void:
	if !mesh_data || !chunk: return
	var mesh_instance = build_mesh_instance(mesh_data, world)
	
	self.delete_chunk(chunk, world)
	world.add_child(mesh_instance)
	chunk.setMeshInstanceId(mesh_instance.get_instance_id())

func build_mesh_instance(mesh_data : Array, owner) -> MeshInstance:
	if !mesh_data: return null
	
	var mesh_instance = MeshInstance.new()
	var mesh : ArrayMesh = ArrayMesh.new()
	var static_body : StaticBody = StaticBody.new()
	
	mesh.add_surface_from_arrays(ArrayMesh.PRIMITIVE_TRIANGLES, mesh_data[0])
	mesh.surface_set_material(0, material)
	
	var polygon_shape : ConcavePolygonShape = ConcavePolygonShape.new()
	polygon_shape.set_faces(mesh_data[1])
	var owner_id = static_body.create_shape_owner(owner)
	static_body.shape_owner_add_shape(owner_id, polygon_shape)

	mesh_instance.add_child(static_body)
	mesh_instance.mesh = mesh
	return mesh_instance
