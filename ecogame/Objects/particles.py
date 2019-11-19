import bpy

object = bpy.context.scene.objects['mug']
particles = object.particle_systems[0].particles

bpy.ops.text.new()

for p in particles:
    bpy.data.texts[-1].write("<particle x='"+str(p.location[0])+"' y='"+str(p.location[1])+"' z='"+str(p.location[2])+"'/>\n")