extends Node

var dims = [4, 4, 4];
var volume = [];

func flatten_index(i,j,k):
	return i + dims[0] * (j + dims[1] * k)

func f(i,j,k):
	return volume[flatten_index(i,j,k)];

func buildMesh():
#	Sweep over 3-axes
	var quads = [];
	
	for z in range(dims[0]*dims[1]*dims[2]):
		volume.append(false)
	
#	volume[0] = true
	volume[1] = true
	
	#Sweep over 3-axes
	for d in range(3):
		var i = 0
		var j = 0
		var k = 0
		var l = 0
		var w = 0
		var h = 0
		var u = (d+1)%3
		var v = (d+2)%3
		var x = [0,0,0]
		var q = [0,0,0]
		var mask = []
		for z in range(dims[u] * dims[v]):
    		mask.append(false)
		q[d] = 1
		x[d] = -1
		while x[d]<dims[d]:
			#Compute mask
			var n = 0;
			x[v]=0
			while x[v]<dims[v]:
				x[u]=0
				while x[u]<dims[u]:
					var a = f(x[0], x[1], x[2]) if 0 <= x[d] else false
					var b = f(x[0]+q[0], x[1]+q[1], x[2]+q[2]) if x[d] < dims[d]-1 else false
					mask[n] = a != b
					x[u] += 1
					n += 1
				x[v] += 1
			#Increment x[d]
			x[d] += 1
			#Generate mesh for mask using lexicographic ordering
			n = 0
			j = 0
			while j<dims[v]:
				i = 0
				while i<dims[u]:
					if mask[n]:
						w = 1
						while mask[n+w] && i+w<dims[u]: w+= 1
						var done = false
						h = 1
						while j+h<dims[v]:
							k = 0
							while k<w:
								if !mask[n+k+h*dims[u]]:
									done = true
									break
								k += 1
							if done:
								break
							h += 1
						x[u] = i
						x[v] = j
						var du = [0,0,0]
						du[u] = w
						var dv = [0,0,0]
						dv[v] = h
						quads.append([
							[x[0],             x[1],             x[2]            ],
							[x[0]+du[0],       x[1]+du[1],       x[2]+du[2]      ],
							[x[0]+du[0]+dv[0], x[1]+du[1]+dv[1], x[2]+du[2]+dv[2]],
							[x[0]      +dv[0], x[1]      +dv[1], x[2]      +dv[2]]
						])
						#Zero-out mask
						l = 0
						while l<h:
							k = 0
							while k<w:
								mask[n+k+l*dims[u]] = false
								k += 1
							l += 1
						#Increment counters and continue
						i += w
						n += w
					else:
						i += 1
						n += 1
					j += 1
	return quads;

# Called every frame. 'delta' is the elapsed time since the previous frame.
#func _process(delta):
#	pass
