extends Spatial
class_name EcoGame

# build thread variables
const TIME_PERIOD = 0.01 # 10ms
const MAX_BUILD_CHUNKS = 24
const MAX_BUILD_SECTIONS = 1

# config
var time = 0
var build_stack : Array = []

func _ready() -> void:
	add_child(Lib.instance)
	Lib.game = self
	
#	var cubeSphere = CubeSphere.new(Vector3(8, 64, 8), 2)
#	add_child(cubeSphere)
	
#	var celestial_body_a = CelestialBody.new(Vector3(64, 64, 64), 16, "a")
#	var celestial_body_b = CelestialBody.new(Vector3(0, 64, 64), 8, "lo_res_world")
#	var celestial_body_a = CelestialBody.new(Vector3(64, 64, 64), 32, "hi_res_world")
	
#	add_child(celestial_body_a)
#	add_child(celestial_body_b)

func _process(delta : float) -> void:
	time += delta
	if time > TIME_PERIOD:
#		var player_pos = $Player.translation
		# Reset timer
		time = 0

func set_path_actor(path, actor_instance_id : int):
	var actor = instance_from_id(actor_instance_id)
	actor.get_node("BehaviourContext").set("path", path)

func draw_debug(geometry : ImmediateGeometry):
	var m = SpatialMaterial.new()
	m.flags_use_point_size = true
	m.vertex_color_use_as_albedo = true
	geometry.set_material_override(m)
	add_child(geometry)

func draw_debug_dots(geometry : ImmediateGeometry):
	var m = SpatialMaterial.new()
	m.flags_use_point_size = true
	m.vertex_color_use_as_albedo = true
	m.params_point_size = 4
	geometry.set_material_override(m)
	add_child(geometry)
