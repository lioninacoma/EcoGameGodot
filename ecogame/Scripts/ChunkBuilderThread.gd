class ChunkBuilderThread:
	extends Node
	
	onready var EcoGame = get_tree().get_root().get_node("EcoGame")
	var gcb = load("res://bin/ecogame.gdns").new()
	
	var _thread
	var _offset
	var _id
	var _finished
	
	var mutex
	
	func _ready():
		mutex = Mutex.new()
	
	func _init(offset):
		self._thread = Thread.new()
		self._offset = offset
		self._id = "ChunkBuilderThread%s" % [offset]
		self._finished = false
	
	func get_id():
		return self._id
	
	func is_finished():
		return self._finished
	
	func generate_mesh():
		if (self._thread.is_active()):
			# Already working
			return
		self._thread.start(self, "_generate", self._offset)
#		_generate(self._offset)
	
	func _generate(offset):
		var volume = PoolByteArray()
		volume.resize(WorldVariables.chunkSizeX * WorldVariables.chunkSizeY * WorldVariables.chunkSizeZ)
		
		# create volume
		var n = 0
		for z in WorldVariables.chunkSizeZ:
			for y in WorldVariables.chunkSizeY:
				for x in WorldVariables.chunkSizeX:
					var noise3DV = Vector3(offset[0] + x, y, offset[2] + z) * WorldVariables.noiseScale
					var noise2DV = Vector2(offset[0] + x, offset[2] + z) * WorldVariables.noiseScale
					volume[n] = 0
					
					if Noise.noise.get_noise_3dv(noise3DV) / 2.0 + 0.5 > 0.6:
						var type = Noise.noise.get_noise_2dv(noise2DV) / 2.0 + 0.5
						type = floor(type * 4)
						volume[n] = type
					n += 1
		
		var gcbVertices = gcb.buildVertices(volume, offset)
		var vertices = PoolVector3Array()
		var uvs = PoolVector2Array()
		
		var txtScale = 1.0 / 64.0
		for v in gcbVertices:
			vertices.push_back(Vector3(v[0], v[1], v[2]))
			uvs.push_back(Vector2(v[3], v[4]))
		
		var st = SurfaceTool.new()
		var meshInstance = MeshInstance.new()
		var mesh = Mesh.new()
		
		st.begin(Mesh.PRIMITIVE_TRIANGLES)
		
		var i = 0
		var j = 0
		var amountIndices = vertices.size() / 2 * 3
		
		while i < amountIndices:
			st.add_index(j + 2)
			st.add_index(j + 1)
			st.add_index(j)
			st.add_index(j)
			st.add_index(j + 3)
			st.add_index(j + 2)
			i += 6
			j += 4
		
		st.set_material(WorldVariables.terrainMaterial)
		
		for v in vertices.size():
			st.add_uv(uvs[v])
			st.add_vertex(vertices[v])
		
		st.generate_normals()
		st.commit(mesh)
		meshInstance.mesh = mesh
		
#		var collisionShape = CollisionShape.new();
#		var staticBody = StaticBody.new()
#		var polygonShape = ConvexPolygonShape.new()
#		polygonShape.set_points(mesh.get_faces())
#		collisionShape.set_shape(polygonShape)
#		meshInstance.add_child(staticBody)
#		staticBody.add_child(collisionShape)
#		meshInstance.create_convex_collision()
		meshInstance.create_trimesh_collision()
		
		self._finished = true
		call_deferred("_generate_done")
		return meshInstance
	
	func _generate_done():
		var staticBody = self._thread.wait_to_finish()
		mutex.lock()
		EcoGame.add_child(staticBody)
		EcoGame.build_thread_ready(self)
		mutex.unlock()
