#!/usr/bin/env python

#based on 'export-sprites.py' and 'glsprite.py' from TCHOW Rainbow; code used is released into the public domain.

#Note: Script meant to be executed from within blender, as per:
#blender --background --python export-scene.py -- <infile.blend> <layer> <outfile.scene>

import sys,re

args = []
for i in range(0,len(sys.argv)):
	if sys.argv[i] == '--':
		args = sys.argv[i+1:]

if len(args) != 2:
	print("\n\nUsage:\nblender --background --python export-scene.py -- <infile.blend>[:layer] <outfile.scene>\nExports the transforms of objects in layer (default 1) to a binary blob, indexed by the names of the objects that reference them.\n")
	exit(1)

infile = args[0]
layer = 3
m = re.match(r'^(.*):(\d+)$', infile)
if m:
	infile = m.group(1)
	layer = int(m.group(2))
outfile = args[1]

print("Will export layer " + str(layer) + " from file '" + infile + "' to scene '" + outfile + "'");

import bpy
import mathutils
import struct
import math

#---------------------------------------------------------------------
#Export scene:

bpy.ops.wm.open_mainfile(filepath=infile)

#Scene file format:
# str0 len < char > * [strings chunk]
# xfh0 len < ... > * [transform hierarchy]
# msh0 len < uint uint uint > [hierarchy point + mesh name]
# cam0 len < uint params > [heirarchy point + camera params]
# lig0 len < uint params > [hierarchy point + light params]

vertices_data = b""
normal_data = b""
poly_data = b""


#source https://blender.stackexchange.com/questions/69881/vertices-coords-and-edges-exporting
#source https://docs.blender.org/api/blender_python_api_2_63_2/bpy.types.Mesh.html
for obj in bpy.data.objects:
	if obj.name == 'WalkMesh':
		for vert in obj.data.vertices:
			for pos in vert.co:
				vertices_data += struct.pack('f', pos)
			for normal in vert.normal:
				normal_data += struct.pack('f', normal)
		for poly in obj.data.polygons:
			poly_data += struct.pack('III', poly.vertices[0], poly.vertices[1], poly.vertices[2])


#write the strings chunk and scene chunk to an output blob:
blob = open(outfile, 'wb')
def write_chunk(magic, data):
	blob.write(struct.pack('4s',magic)) #type
	blob.write(struct.pack('I', len(data))) #length
	blob.write(data)

write_chunk(b'vtc0', vertices_data)
write_chunk(b'nml0', normal_data)
write_chunk(b'ply0', poly_data)


print("Wrote " + str(blob.tell()) + " bytes to '" + outfile + "'")
blob.close()
