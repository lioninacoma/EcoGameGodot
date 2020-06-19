extends MeshInstance
class_name QuadTreeMesh

var root: QuadTreeQuad
export(NodePath) var camera_path
onready var camera : Spatial = get_node(camera_path)
var quad_data : Dictionary
var camera_pos_before = null
var SurfaceNoiseShader = preload("res://SurfaceNoise.shader")
var material : ShaderMaterial
var noise : OpenSimplexNoise
var noise_texture : NoiseTexture
const TIME_PERIOD = 0.05 # 50ms
var time = 0

func _init():
	mesh = ArrayMesh.new()
	quad_data = {
		"parent": null,
		"quad": null,
		"child_index": 0,
		"level": 3,
		"xorg": 0,
		"zorg": 0
	}
	root = QuadTreeQuad.new(quad_data)
	
	noise = OpenSimplexNoise.new()
	noise.set_seed(6161)
	noise.set_octaves(4)
	noise.set_period(128.0)
	noise.set_persistence(0.5)
	noise_texture = NoiseTexture.new()
	noise_texture.set_noise(noise)
	material = ShaderMaterial.new()
	material.set_shader(SurfaceNoiseShader)
	material.set_shader_param("height_scale", 3.0)
	material.set_shader_param("noise", noise_texture)

func _process(delta):
	if !camera_path: return
	
	if !camera:
		camera = get_node(camera_path)
		return
	
	var camera_pos = camera.global_transform.origin
	
	time += delta
	if time > TIME_PERIOD && (camera_pos_before == null 
		|| camera_pos_before != camera_pos):
		camera_pos_before = camera_pos
		rebuild_geom()
		time = 0

func create_mesh():
	if !camera: return
	
	var arrays = []
	var vertex_array = PoolVector3Array()
	var normal_array = PoolVector3Array()
	var index_array = PoolIntArray()
	
	var vertices = []
	var indices = []
	var counts = [0, 0]
	
	vertices.resize(4194304)
	indices.resize(4194304)
	
	root.update(quad_data, to_local(camera.global_transform.origin))
	root.build_mesh(quad_data, vertices, indices, counts)
	
	vertex_array.resize(counts[0])
	normal_array.resize(counts[0])
	index_array.resize(counts[1])

	for v in range(counts[0]):
		vertex_array[v] = vertices[v]
		normal_array[v] = Vector3(0, 1, 0)
	for i in range(counts[1]):
		index_array[i] = indices[i]

	arrays.resize(Mesh.ARRAY_MAX)
	arrays[Mesh.ARRAY_VERTEX] = vertex_array
	arrays[Mesh.ARRAY_NORMAL] = normal_array
	arrays[Mesh.ARRAY_INDEX] = index_array

	mesh.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, arrays)
	mesh.surface_set_material(0, material)

func add_normal(src, normal) -> Vector3:
	return (src + normal).normalized()

func rebuild_geom():
	if mesh && mesh.get_surface_count() > 0:
		mesh.surface_remove(0)
	create_mesh()
