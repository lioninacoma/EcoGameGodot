class ChunkBuilderThread:
	extends Node
	
	onready var EcoGame = get_tree().get_root().get_node("EcoGame")
	
	var gcb = load("res://bin/ecogame.gdns").new()
	
	var _thread
	var _chunk
	var _id
	
	func _init(chunk):
		self._thread = Thread.new()
		self._chunk = chunk
		self._id = "ChunkBuilderThread%s" % [chunk.get_offset()]

	func get_id():
		return self._id
	
	func get_chunk():
		return self._chunk
	
	func generate_mesh():
		if (self._thread.is_active()):
			return
		self._thread.start(self, "_generate", self._chunk)
#		self._generate(self._chunk)
	
	func _generate(chunk):
		var staticBody = StaticBody.new()
		var polygonShape = ConcavePolygonShape.new()
		var meshInstance = MeshInstance.new()
		var st = SurfaceTool.new()
		var mesh = Mesh.new()
		
		# if not update, build voxels
		if chunk.get_volume() == null:
			chunk.build_volume()
		
		var vertices = gcb.buildVertices(chunk.get_volume(), chunk.get_offset())
		var collFaces = PoolVector3Array()
		
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
		
		print("%s done generating!" % [self.get_id()])
		call_deferred("_generate_done")
		return meshInstance
		
	func _generate_done():
		var meshInstance = self._thread.wait_to_finish()
		
		# if update, remove previous mesh instance from scene tree
		if _chunk.get_mesh_instance() != null:
			EcoGame.remove_child(_chunk.get_mesh_instance())
			
		_chunk.set_mesh_instance(meshInstance)
		
		# add mesh instance to scene tree
		EcoGame.add_child(meshInstance)
		EcoGame.build_thread_ready(self)
		
		# free greedy chunk builder and chunk builder thread
		call_deferred("free")
		gcb.call_deferred("free")
	