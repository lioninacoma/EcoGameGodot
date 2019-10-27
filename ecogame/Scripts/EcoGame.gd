extends Spatial

onready var ChunkBuilderThread = load("res://Scripts/ChunkBuilderThread.gd")
onready var fpsLabel = get_node('FPSLabel')

# build thread variables
onready var buildStack = []
onready var maxAmountBuildJobs = 8
onready var amountBuildJobs = 0

func _ready():
	_build_world()

func _build_world():
	# generate world map
	for z in WorldVariables.worldSize:
		for x in WorldVariables.worldSize:
			var offset = [x * WorldVariables.chunkSizeX, 0, z * WorldVariables.chunkSizeZ]
			buildStack.push_back(offset);

func _process(delta):
	fpsLabel.set_text(str(Engine.get_frames_per_second()))
	
	for i in 8:
		if buildStack.size() > 0 && amountBuildJobs < maxAmountBuildJobs:
#			print(amountBuildJobs)
			amountBuildJobs += 1
			var offset = buildStack.pop_front()
			var chunkBuilder = ChunkBuilderThread.ChunkBuilderThread.new(offset)
			add_child(chunkBuilder)
			chunkBuilder.generate_mesh()
		else:
			break

func build_thread_ready(chunkBuilder):
	amountBuildJobs -= 1
	remove_child(chunkBuilder)
	if chunkBuilder.is_finished():
		print("%s finished" % [chunkBuilder.get_id()])
