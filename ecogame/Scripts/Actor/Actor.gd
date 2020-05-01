extends KinematicBody
class_name Actor

var voxel_world
var velocity = Vector3()
var acceleration = Vector3()

# walk variables
const MAX_SPEED = 3
const TIME_PERIOD = 0.05 # 50ms
var gravity = Vector3(0, -9.8, 0)
var time = 0
var source_basis = null
var target_basis = null
var lerp_i = 0

onready var eco_game = get_tree().get_root().get_node("EcoGame")
onready var context = $BehaviourContext

func _ready():
	pass

func _process(delta : float) -> void:
	update(delta)

func set_voxel_world(voxel_world):
	self.voxel_world = voxel_world

func get_voxel_world():
	return voxel_world

func apply_force(force : Vector3):
	acceleration += force

func update(delta : float) -> void:
	velocity += acceleration
	
	if velocity.length() >= MAX_SPEED:
		velocity = velocity.normalized()
		velocity *= MAX_SPEED
	
	move_and_slide(velocity, Vector3(0, 1, 0))
	acceleration *= 0
	
	lerp_i += 3 * delta
	time += delta
	
	if target_basis:
		if lerp_i <= 1.0:
			transform.basis = source_basis.slerp(target_basis, lerp_i)
			return
		transform.basis = target_basis
	
	if velocity.length() <= 0:
		return
	
	if time > TIME_PERIOD:
		time = 0
		lerp_i = 0
		
		var cog = voxel_world.get_parent().get_center_of_gravity()
		var g = (transform.origin - cog).normalized()
		var yB = global_transform.basis.y
		var o = global_transform.origin
		var start = o + yB
		var end   = o - yB
		var space_state = eco_game.get_world().direct_space_state
		var result = space_state.intersect_ray(start, end)
		
		if result:
			var mesh = result.collider.get_parent().mesh
			var arrays = mesh.surface_get_arrays(0)
			var vertices = arrays[Mesh.ARRAY_VERTEX]
			var indices  = arrays[Mesh.ARRAY_INDEX]
			var normals  = arrays[Mesh.ARRAY_NORMAL]
			
			start = voxel_world.to_local(start)
			end   = voxel_world.to_local(end)
			var normal = result.normal
			
			for i in range(0, indices.size(), 3):
				var a_i = indices[i]
				var b_i = indices[i + 1]
				var c_i = indices[i + 2]
				var a = vertices[a_i]
				var b = vertices[b_i]
				var c = vertices[c_i]
				if Geometry.segment_intersects_triangle(start, end, a, b, c):
					normal = (normals[a_i] + normals[b_i] + normals[c_i]).normalized()
					break
			
			rotate_to(normal, delta)
#			rotate_to(g + normal, delta)

func rotate_to(normal : Vector3, delta : float):
	var D = normal
	var W0 = Vector3(-D.y, D.x, 0)
	var U0 = W0.cross(D)
	source_basis = Basis(transform.basis)
	target_basis = Basis(W0, D, U0).orthonormalized()
	

func set_rotation_to(normal : Vector3):
	var D = normal
	var W0 = Vector3(-D.y, D.x, 0)
	var U0 = W0.cross(D)
	transform.basis = Basis(W0, D, U0).orthonormalized()
