extends WorldObject
class_name IronFactory

signal iron_mine_unset(id)

var transporters = []
var workers = []

var idle_transporters = []
var idle_workers = []

var iron_mine = null

func _init(position).(position):
	data.child_data = {
		"resources": {
			"funds": 1000,
			"iron": 0,
			"transporters": [
				{ "position": position },
				{ "position": position },
				{ "position": position },
				{ "position": position }
			],
			"workers": [
				{ "position": position },
				{ "position": position },
				{ "position": position },
				{ "position": position },
				{ "position": position },
				{ "position": position },
				{ "position": position },
				{ "position": position },
				{ "position": position },
				{ "position": position }
			]
		}
	}

func _ready():
	for d in data.child_data.resources.transporters:
		var transporter = Transporter.new(d.position)
		transporters.push_back(transporter)
		get_world().add_object(transporter)
		transporter.connect("changing_state", self, "_on_transporter_changing_state")
	
	for d in data.child_data.resources.workers:
		var worker = Worker.new(d.position)
		workers.push_back(worker)
		get_world().add_object(worker)
		worker.connect("changing_state", self, "_on_worker_changing_state")
		worker.connect("storing_iron", self, "_on_worker_storing_iron")
	pass

func _on_transporter_changing_state(id, state) -> void:
	if state == STATE.IDLE:
		idle_transporters.push_back(id)
		print("transporter %s is idle"%[id])
	else:
		var idx = idle_transporters.find(id)
		if idx != -1:
			idle_transporters.remove(idx)

func _on_worker_changing_state(id, state) -> void:
	if state == STATE.IDLE:
		idle_workers.push_back(id)
		print("worker %s is idle"%[id])
	else:
		var idx = idle_workers.find(id)
		if idx != -1:
			idle_workers.remove(idx)
	
	if state == STATE.GATHERING_IRON:
		print("worker %s is gathering iron"%[id])

func _on_worker_storing_iron(id, amount_iron) -> void:
	data.child_data.resources.iron += amount_iron
	if data.child_data.resources.iron % 100 == 0:
		print ("resources.iron: %s"%[data.child_data.resources.iron])
	
	var transport_amount_iron = 500
	if data.child_data.resources.iron >= transport_amount_iron:
		if !idle_transporters.empty():
			data.child_data.resources.iron -= transport_amount_iron
			var transporter = instance_from_id(idle_transporters[0])
			transporter.transport_iron_to_base(transport_amount_iron)

func _process(delta : float) -> void:
	if iron_mine == null:
		emit_signal("iron_mine_unset", self.get_instance_id())
	
	if iron_mine != null:
		for worker_id in idle_workers:
			var worker = instance_from_id(worker_id)
			worker.gather_iron(iron_mine)

#var inventory = Inventory.new()
#
## Called when the node enters the scene tree for the first time.
#func _ready():
#	inventory.add_ressource("worker", 		0, 10, 0, 0, [], 0)
#	inventory.add_ressource("transporter", 	0, 2, 100, 50, ["worker"], 1)
#	inventory.add_ressource("iron", 		0, 100, 0, 20, ["worker", "transporter"], 2)
#	inventory.set_funds(1000)
#	inventory.calc_needs()
#
#class Inventory:
#	var funds = 0
#	var ressources = {}
#
#	func add_ressource(name, count, max_count, purchase_value, sell_value, requirements, priority):
#		if requirements == null:
#			requirements = []
#		ressources[name] = Ressource.new(count, max_count, purchase_value, sell_value, requirements, priority)
#
#	func set_funds(funds):
#		self.funds = funds
#
#	func get_funds():
#		return funds
#
#	func calc_needs():
#		var max_t : float = 0.0
#		var t_s = []
#		for key in ressources.keys():
#			var res = ressources[key]
#			var t : float = res.priority
#			for val in ressources.values():
#				if key in val.requirements:
#					t += val.priority
#			t *= (1.0 - min(res.count / res.max_count, 1.0))
#			if funds > 0:
#				t *= (1.0 - min(float(res.purchase_value) / funds, 1.0))
#			else:
#				t = 0.0
#			max_t += t
#			t_s.push_back(t)
#
#		var i = 0
#		for key in ressources.keys():
#			var res = ressources[key]
#			res.need = t_s[i] / max_t
#			i += 1
#		pass
#
#class Ressource:
#	var count
#	var max_count
#	var purchase_value = 0
#	var sell_value = 0
#	var need = 0
#	var requirements
#	var priority
#
#	func _init(count, max_count, purchase_value, sell_value, requirements, priority):
#		self.count = count
#		self.max_count = max_count
#		self.purchase_value = purchase_value
#		self.sell_value = sell_value
#		self.requirements = requirements
#		self.priority = priority
