extends Node
class_name ChunkBuilder

onready var EcoGame : Node = get_parent()

# dependencies
var EcoGameLib = load("res://bin/ecogame.gdns")

# globals
onready var WorldVariables : Node = get_node("/root/WorldVariables")

var _threadPool : ThreadPool
var _amountActive : int

func _init() -> void:
	self._threadPool = ThreadPool.new(8)

func get_amount_active() -> int:
	return self._amountActive

func build_chunk(chunk : Chunk) -> void:
	_threadPool.submit_task(self, "_build", [chunk])
#	_build([chunk])
	_amountActive += 1

func _build(userdata) -> void:
	if !userdata || userdata.empty(): return
	
	var startTime = OS.get_ticks_msec()
	
	var eco = EcoGameLib.new()
	var chunk = userdata[0]
	var staticBody = StaticBody.new()
	var polygonShape = ConcavePolygonShape.new()
	var meshInstance = MeshInstance.new()
	var st = SurfaceTool.new()
	var mesh = Mesh.new()
	var volume = chunk.get_volume()
	
	# if not update, build voxels
	if !volume:
#		volume = chunk.build_volume()
		volume = eco.buildVolume(chunk.get_offset(), WorldVariables.NOISE_SEED)
		chunk.set_volume(volume)
	
	var collFaces = PoolVector3Array()
	
	var startTimeBV = OS.get_ticks_msec()
	var vertices = eco.buildVertices(chunk.get_offset(), volume)
	
	st.begin(Mesh.PRIMITIVE_TRIANGLES)
	
	var i = 0
	var j = 0
	var amountIndices = vertices.size() / 2 * 3
	collFaces.resize(amountIndices)
	
	while i < amountIndices:
		st.add_index(j + 2)
		st.add_index(j + 1)
		st.add_index(j)
		st.add_index(j)
		st.add_index(j + 3)
		st.add_index(j + 2)
		collFaces[i] 	 = vertices[j + 2][0]
		collFaces[i + 1] = vertices[j + 1][0]
		collFaces[i + 2] = vertices[j][0]
		collFaces[i + 3] = vertices[j][0]
		collFaces[i + 4] = vertices[j + 3][0]
		collFaces[i + 5] = vertices[j + 2][0]
		i += 6
		j += 4
	
	st.set_material(WorldVariables.terrainMaterial)
	
	for v in vertices:
		st.add_uv(v[1])
		st.add_vertex(v[0])
	
	st.generate_normals()
	st.commit(mesh)
	meshInstance.mesh = mesh
	
	polygonShape.set_faces(collFaces)
	var ownerId = staticBody.create_shape_owner(staticBody)
	staticBody.shape_owner_add_shape(ownerId, polygonShape)
	meshInstance.add_child(staticBody)
	
	print("%s BUILT in %s ms!"%[chunk, OS.get_ticks_msec() - startTime])
	call_deferred("_build_done", chunk, meshInstance)
	
func _build_done(chunk : Chunk, meshInstance : MeshInstance) -> void:
	# if update, remove previous mesh instance from scene tree
	if chunk.get_mesh_instance() != null:
		EcoGame.remove_child(chunk.get_mesh_instance())
	
	chunk.set_mesh_instance(meshInstance)
	
	# add mesh instance to scene tree
	EcoGame.add_child(meshInstance)
	_amountActive -= 1
	