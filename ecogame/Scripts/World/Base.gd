extends WorldObject
class_name Base

var iron_factories = []
var iron_mines = []

func _init(position).(position):
	data.child_data = {
		"buildings": {
			"iron_factories": [
				{ "position": Vector3(100, 0, 100) }
			],
			"iron_mines": [
				{ "position": Vector3(150, 0, 50) }
			]
		}
	}

# Called when the node enters the scene tree for the first time.
func _ready():
	for d in data.child_data.buildings.iron_factories:
		var iron_factory = IronFactory.new(d.position)
		iron_factories.push_back(iron_factory)
		get_world().add_object(iron_factory)
		iron_factory.connect("changing_state", self, "_on_iron_factory_changing_state")
		iron_factory.connect("iron_mine_unset", self, "_on_iron_factory_iron_mine_unset")
	
	for d in data.child_data.buildings.iron_mines:
		var iron_mine = IronMine.new(d.position)
		iron_mines.push_back(iron_mine)
		get_world().add_object(iron_mine)
		iron_mine.connect("changing_state", self, "_on_iron_mine_changing_state")

func _on_iron_factory_changing_state(id, state) -> void:
	if state == STATE.IDLE:
		print("iron_factory %s is idle"%[id])

func _on_iron_factory_iron_mine_unset(id) -> void:
	print("iron_factory %s has unset iron_mine"%[id])
	var iron_factory = instance_from_id(id)
	var iron_mine = get_closest_iron_mine(iron_factory.position)
	if iron_mine != null:
		iron_factory.iron_mine = iron_mine
		print("iron_factory %s set iron_mine %s"%[id, iron_mine.get_instance_id()])

func _on_iron_mine_changing_state(id, state) -> void:
	if state == STATE.IDLE:
		print("iron_mine %s is idle"%[id])

func get_closest_iron_mine(position):
	var closest_dist = INF
	var closest_mine = null
	for iron_mine in iron_mines:
		var current_position = iron_mine.position
		var current_dist = current_position.distance_to(position)
		if current_dist < closest_dist:
			closest_dist = current_dist
			closest_mine = iron_mine
	return closest_mine
