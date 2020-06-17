class_name QuadTreeQuad

# child_index mapping:
# +-+-+
# |1|0|
# +-+-+
# |2|3|
# +-+-+
#
# verts mapping:
# 1-0
# | |
# 2-3
#
# vertex mapping:
# +-2-+
# | | |
# 3-0-1
# | | |
# +-4-+
var child : Array

# quad_data dict:
# {
#	 "parent": null,
#	 "quad": null,
#	 "child_index": 0,
#	 "level": 0,
#	 "xorg": 0,
#	 "zorg": 0,
# }
var quad_data : Dictionary

# bits 0-7: e, n, w, s, ne, nw, sw, se
var enabled_flags : int
var sub_enabled_count : Array

# data for array mesh
var arrays : Array

func create_instance(quad_data):
	var obj = load(get_script().resource_path).new(quad_data)
	return obj

func _init(quad_data):
	quad_data.quad = self
	
	self.child = []
	self.quad_data = quad_data
	self.enabled_flags = 0
	self.sub_enabled_count = [0, 0]
	
	child.resize(4)

func build_mesh(quad_data, vertices, indices, counts):
	var half = 1 << quad_data.level
	var whole = 2 << quad_data.level

	var flags = 0
	var mask = 1
	for i in range(4):
		if enabled_flags & (16 << i):
			var q = setup_child_quad_data(quad_data, i)
			child[i].build_mesh(q, vertices, indices, counts)
		else:
			flags |= mask
		
	if flags == 0: return
#	print ("x: %s, z: %s, i: %s, lvl: %s, enabled: %s" 
#		% [quad_data.xorg, quad_data.zorg, quad_data.child_index, quad_data.level, enabled_flags])
#	if quad_data.xorg == 4 && quad_data.zorg == 4:
#		pass
	
	# Init vertex data.
	counts[0] = init_vert(counts[0], quad_data.xorg + half, 0, quad_data.zorg + half, vertices)
	counts[0] = init_vert(counts[0], quad_data.xorg + whole, 0, quad_data.zorg + half, vertices)
	counts[0] = init_vert(counts[0], quad_data.xorg + whole, 0, quad_data.zorg, vertices)
	counts[0] = init_vert(counts[0], quad_data.xorg + half, 0, quad_data.zorg, vertices)
	counts[0] = init_vert(counts[0], quad_data.xorg, 0, quad_data.zorg, vertices)
	counts[0] = init_vert(counts[0], quad_data.xorg, 0, quad_data.zorg + half, vertices)
	counts[0] = init_vert(counts[0], quad_data.xorg, 0, quad_data.zorg + whole, vertices)
	counts[0] = init_vert(counts[0], quad_data.xorg + half, 0, quad_data.zorg + whole, vertices)
	counts[0] = init_vert(counts[0], quad_data.xorg + whole, 0, quad_data.zorg + whole, vertices)

	# Make the list of triangles to draw.
	if (enabled_flags & 1) == 0: counts[1] = tri(counts[1], 0, 8, 2, indices);
	else:
		if flags & 8: counts[1] = tri(counts[1], 0, 8, 1, indices)
		if flags & 1: counts[1] = tri(counts[1], 0, 1, 2, indices)
	if (enabled_flags & 2) == 0: counts[1] = tri(counts[1], 0, 2, 4, indices)
	else:
		if flags & 1: counts[1] = tri(counts[1], 0, 2, 3, indices)
		if flags & 2: counts[1] = tri(counts[1], 0, 3, 4, indices)
	if (enabled_flags & 4) == 0: counts[1] = tri(counts[1], 0, 4, 6, indices)
	else:
		if flags & 2: counts[1] = tri(counts[1], 0, 4, 5, indices)
		if flags & 4: counts[1] = tri(counts[1], 0, 5, 6, indices)
	if (enabled_flags & 8) == 0: counts[1] = tri(counts[1], 0, 6, 8, indices)
	else:
		if flags & 4: counts[1] = tri(counts[1], 0, 6, 7, indices)
		if flags & 8: counts[1] = tri(counts[1], 0, 7, 8, indices)

func tri(index, a, b, c, indices):
	indices[index] = a
	indices[index + 1] = b
	indices[index + 2] = c
	index += 3
	return index

func init_vert(index, x, y, z, vertices):
	vertices[index] = x
	vertices[index + 1] = y
	vertices[index + 2] = z
	index += 3
	return index

func update(quad_data, camera_location : Vector3):
	var half = 1 << quad_data.level
	var whole = half << 1
	
	if ((enabled_flags & 1) == 0 && vertex_test(quad_data.xorg + whole,
		quad_data.zorg + half, camera_location)): 
		enable_edge_vertex(0, false, quad_data) # East vert
	if ((enabled_flags & 8) == 0 && vertex_test(quad_data.xorg + half, 
		quad_data.zorg + whole, camera_location)): 
		enable_edge_vertex(3, false, quad_data) # South vert
	if quad_data.level > 0:
		if (enabled_flags & 32) == 0:
			if box_test(quad_data.xorg, quad_data.zorg, half, camera_location):
				enable_child(1, quad_data) # nw child
		if (enabled_flags & 16) == 0:
			if box_test(quad_data.xorg + half, quad_data.zorg, half, camera_location):
				enable_child(0, quad_data) # ne child
		if (enabled_flags & 64) == 0:
			if box_test(quad_data.xorg, quad_data.zorg + half, half, camera_location):
				enable_child(2, quad_data) # sw child
		if (enabled_flags & 128) == 0:
			if box_test(quad_data.xorg + half, quad_data.zorg + half, half, camera_location):
				enable_child(3, quad_data) # se child
		
		# Recurse into child quadrants as necessary.
		if enabled_flags & 32:
			var q = setup_child_quad_data(quad_data, 1)
			child[1].update(q, camera_location)
		if enabled_flags & 16:
			var q = setup_child_quad_data(quad_data, 0)
			child[0].update(q, camera_location)
		if enabled_flags & 64:
			var q = setup_child_quad_data(quad_data, 2)
			child[2].update(q, camera_location)
		if enabled_flags & 128:
			var q = setup_child_quad_data(quad_data, 3)
			child[3].update(q, camera_location)

#	print ("x: %s, z: %s, i: %s, lvl: %s, enabled: %s" 
#		% [quad_data.xorg, quad_data.zorg, quad_data.child_index, quad_data.level, enabled_flags])
	
	#Test for disabling.  East, South, and center.
#	if ((enabled_flags & 1) && sub_enabled_count[0] == 0 
#		&& !vertex_test(quad_data.xorg + whole, quad_data.zorg + half, camera_location)):
#		enabled_flags &= ~1
#		var s = get_neighbour(0, quad_data)
#		if s: s.enabled_flags &= ~4
#	if ((enabled_flags & 8) && sub_enabled_count[1] == 0 
#		&& !vertex_test(quad_data.xorg + half, quad_data.zorg + whole, camera_location)):
#		enabled_flags &= ~8
#		var s = get_neighbour(3, quad_data)
#		if s: s.enabled_flags &= ~2
#	if (enabled_flags == 0
#		&& quad_data.parent != null
#		&& !box_test(quad_data.xorg, quad_data.zorg, whole, camera_location)):
#		# Disable ourself.
#		# nb: possibly deletes 'this'.
#		quad_data.parent.quad.notify_child_disable(quad_data.parent, quad_data.child_index)

func enable_edge_vertex(index, increment_count, quad_data):
	if (enabled_flags & (1 << index)) && !increment_count: return
	
	enabled_flags |= 1 << index
	if increment_count && (index == 0 || index == 3):
		sub_enabled_count[index & 1] += 1
	
	var p = self
	var pqd = quad_data
	var ct = 0
	var stack = []
	stack.resize(32)
	
	# Travel upwards through the tree, looking for the parent in common with our desired neighbor.
	# Remember the path through the tree, so we can travel down the complementary path to get to the neighbor.
	while true:
		var ci = pqd.child_index
		
		if pqd.parent == null || pqd.parent.quad == null:
			# Neighbor doesn't exist (it's outside the tree), so there's no alias vertex to enable.
			return;
		p = pqd.parent.quad
		pqd = pqd.parent
		
		var same_parent = bool((index - ci) & 2)
		
		ci = ci ^ 1 ^ ((index & 1) << 1) # Child index of neighbor node.
		
		stack[ct] = ci
		ct += 1
		
		if same_parent: break
	
	p = p.enable_descendant(ct, stack, pqd)
	
	# Finally: enable the vertex on the opposite edge of our neighbor, the alias of the original vertex.
	index ^= 2
	p.enabled_flags |= (1 << index)
	if increment_count && (index == 0 || index == 3):
		p.sub_enabled_count[index & 1] += 1

func enable_descendant(count, path, quad_data):
	count -= 1
	var child_index = path[count]

	if (enabled_flags & (16 << child_index)) == 0:
		enable_child(child_index, quad_data)
	
	if count > 0:
		var q = setup_child_quad_data(quad_data, child_index)
		return child[child_index].enable_descendant(count, path, q);
	else:
		return child[child_index]

func enable_child(index, quad_data):
	if (enabled_flags & (16 << index)) == 0:
		enabled_flags |= (16 << index)
		enable_edge_vertex(index, true, quad_data)
		enable_edge_vertex((index + 1) & 3, true, quad_data)
		
		if !child[index]:
			create_child(index, quad_data)

func notify_child_disable(quad_data, index):
	enabled_flags &= ~(16 << index)
	
	var s = null
	
	if index & 2: s = self
	else: s = get_neighbour(1, quad_data)
	if s:
		s.sub_enabled_count[1] -= 1
	
	if index == 1 || index == 2: s = get_neighbour(2, quad_data)
	else: s = self
	if s:
		s.sub_enabled_count[0] -= 1
	
	child[index] = null

func get_neighbour(dir, quad_data):
	if quad_data.parent == 0: return 0
	
	var p = null
	
	var index = quad_data.child_index ^ 1 ^ ((dir & 1) << 1)
	var same_parent = bool((dir - quad_data.child_index) & 2)
	
	if same_parent:
		p = quad_data.parent.quad
	else:
		p = quad_data.parent.quad.get_neighbor(dir, quad_data.parent)
		if p == 0: return 0
	
	var n = p.child[index]
	return n

func create_child(index, quad_data):
	if !child[index]:
		var q = setup_child_quad_data(quad_data, index)
		child[index] = create_instance(q)
		pass

var detail_threshold = 41

func box_test(x, z, size, camera_location):
	var half = size * 0.5
	var dx = abs(x + half - camera_location.x) - half
	var dy = abs(camera_location.y)
	var dz = abs(z + half - camera_location.z) - half
	var d = dx
	if dy > d: d = dy
	if dz > d: d = dz
#	print ("x: %s, z: %s, size: %s, cam: %s, distance: %s" 
#		% [x, z, size, camera_location, d])
	return detail_threshold > d

func vertex_test(x, z, camera_location):
#	var d = Vector3(x, 0, z).distance_to(camera_location)
	var dx = abs(x - camera_location.x)
	var dy = abs(camera_location.y)
	var dz = abs(z - camera_location.z)
	var d = dx
	if dy > d: d = dy
	if dz > d: d = dz
	return detail_threshold > d

func setup_child_quad_data(quad_data, child_index : int) -> Dictionary:
	var half = 1 << quad_data.level
	var child_quad_data = {}
	
	child_quad_data.parent = quad_data
	child_quad_data.quad = child[child_index]
	child_quad_data.level = quad_data.level - 1
	child_quad_data.child_index = child_index
	
	match child_index:
		0:
			child_quad_data.xorg = quad_data.xorg + half
			child_quad_data.zorg = quad_data.zorg
		1:
			child_quad_data.xorg = quad_data.xorg
			child_quad_data.zorg = quad_data.zorg
		2:
			child_quad_data.xorg = quad_data.xorg
			child_quad_data.zorg = quad_data.zorg + half
		3:
			child_quad_data.xorg = quad_data.xorg + half
			child_quad_data.zorg = quad_data.zorg + half
	
	return child_quad_data
