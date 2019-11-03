extends Node

func segment3DIntersections(start : Vector3, end : Vector3, first : bool, intersection : FuncRef, list : Array) -> Array:
	var it = 0
	var itMax = 32000
	
	var gx0idx = floor(start.x)
	var gy0idx = floor(start.y)
	var gz0idx = floor(start.z)
	
	var gx1idx = floor(end.x)
	var gy1idx = floor(end.y)
	var gz1idx = floor(end.z)
	
	var sx = 1 if gx1idx > gx0idx else (0 if gx1idx == gx0idx else -1)
	var sy = 1 if gy1idx > gy0idx else (0 if gy1idx == gy0idx else -1)
	var sz = 1 if gz1idx > gz0idx else (0 if gz1idx == gz0idx else -1)
	
	var gx = gx0idx
	var gy = gy0idx
	var gz = gz0idx
	
	# Planes for each axis that we will next cross
	var gxp = gx0idx + (1 if gx1idx > gx0idx else 0);
	var gyp = gy0idx + (1 if gy1idx > gy0idx else 0);
	var gzp = gz0idx + (1 if gz1idx > gz0idx else 0);
	
	# Only used for multiplying up the error margins
	var vx = 1.0 if end.x == start.x else end.x - start.x
	var vy = 1.0 if end.y == start.y else end.y - start.y
	var vz = 1.0 if end.z == start.z else end.z - start.z
	
	# Error is normalized to vx * vy * vz so we only have to multiply up
	var vxvy = vx * vy
	var vxvz = vx * vz
	var vyvz = vy * vz
	
	# Error from the next plane accumulators, scaled up by vx*vy*vz
	# gx0 + vx * rx === gxp
	# vx * rx === gxp - gx0
	# rx === (gxp - gx0) / vx
	var errx = (gxp - start.x) * vyvz
	var erry = (gyp - start.y) * vxvz
	var errz = (gzp - start.z) * vxvy
	
	var derrx = sx * vyvz
	var derry = sy * vxvz
	var derrz = sz * vxvy
	
	while (true):
		var p = intersection.call_func(gx, gy, gz)
		if p != null && !list.has(p):
			list.push_back(p)
			if (first): return list
		
		if gx == gx1idx && gy == gy1idx && gz == gz1idx: break
		
		# Which plane do we cross first?
		var xr = abs(errx)
		var yr = abs(erry)
		var zr = abs(errz)
		
		if sx != 0 && (sy == 0 || xr < yr) && (sz == 0 || xr < zr):
		    gx += sx
		    errx += derrx
		elif sy != 0 && (sz == 0 || yr < zr):
		    gy += sy
		    erry += derry
		elif sz != 0:
		    gz += sz;
		    errz += derrz;
		
		it += 1
		if it >= itMax: break
	
	return list;
