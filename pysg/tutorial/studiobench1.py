#Copyright 2021 Intel Corporation
#SPDX-License-Identifier: Apache-2.0

"""
An example of using python to benchmark studio rendering of vtk data files.
Example use:
 python3 studiobench1.py -numFrames 200 -colors 2 0 0 0 1 1 1 data_32.vti
"""

import sys, numpy, math, time, atexit, resource
import pysg as sg
from pysg import Any, vec3f, Data, FileName, Importer, MaterialRegistry, ArcballCamera, vec2i, box3f, vec2f

args = sg.init(sys.argv)

sg.loadPlugin("vtk")

#optional command line arguments
scalarRange = None
colors = None
opacities = None
W = 1024
H = 768
numFrames = 100
  
for idx in range(0, len(args)):
  a = args[idx]
  if a == "-range":
    scalarRange = vec2f(float(args[idx+1]), float(args[idx+2]))
  if a == "-colors":
    nc = int(args[idx+1])
    colors = []
    for x in range(0,nc):
      R = float(args[idx+2+x*3+0])
      G = float(args[idx+2+x*3+1])
      B = float(args[idx+2+x*3+2])
      colors.append([R,G,B])
  if a == "-opacities":
    no = int(args[idx+1])
    opacities = []
    for x in range(0,no):
      O = float(args[idx+2+x+0])
      opacities.append([O])
  if a == "-res":
    W = int(args[idx+1])
    H = int(args[idx+2])
  if a == "-numFrames":
    numFrames = int(args[idx+1])

#figure out what vtk file to open and what it is like
filename = args[len(args)-1]
volumename = filename.split("/")[-1]
extps = volumename.rfind(".")
filetype = volumename[extps:]
volumename = volumename[0:extps]
  
window_size = vec2i(W, H)
aspect = Any(float(W) / H)
  
frame = sg.Frame()
frame.immediatelyWait = True
frame.createChild("windowSize", "vec2i", Any(window_size))
  
renderer = sg.Renderer("scivis")
frame.createChildAs("renderer", "renderer_scivis")
  
world = frame.child("world")
  
lightsMan = frame.child("lights")
baseMaterialRegistry = frame.child("baseMaterialRegistry")
  
importer = sg.getImporter(world, FileName(filename))
importer.setLightsManager(lightsMan)
importer.setMaterialRegistry(baseMaterialRegistry)
importer.importScene()

if filetype == ".vti" or filetype == ".vtu":
  #it is a good idea to standardadize LUT range to keep picture consistent
  #traverse scenegraph to the place we need to hange LUT on
  importer = world.child(volumename+"_importer")
  transform = importer.child("xfm")
  tfOrig = transform.child("transferFunction")
  if scalarRange is not None:
    tfOrig.remove("valueRange")
    tfOrig.createChild("valueRange", "vec2f", Any(scalarRange))
  if colors is not None:
    npColors = numpy.array(colors,dtype=numpy.float32)
    tfOrig.remove("color")
    tfOrig.createChildData("color", Data(npColors))
  if opacities is not None:
    npOpacities = numpy.array(opacities,dtype=numpy.float32)
    tfOrig.remove("opacity")
    tfOrig.createChildData("opacity", Data(npOpacities))

world.render()
bounds = world.bounds()
  
arcballCamera = ArcballCamera(bounds, window_size)
cam = frame.child("camera")
cam.createChild("aspect", "float", aspect)
sg.updateCamera(cam, arcballCamera)
  
# First frame will be "navigation" resolution.
# Render again for full sized frame.
frame.startNewFrame(bool(0))
frame.startNewFrame(bool(0))
frame.waitOnFrame()
frame.saveFrame("benchmark_initial.png", 0)
elapsed = 0.0
min = 1000000.0
max = -1.0
for frameNum in range(0,numFrames):
  ts = time.time()
  arcballCamera.rotate(vec2f(-2.0/numFrames, 0.0),vec2f(2.0/numFrames, 0.0))
  sg.updateCamera(cam, arcballCamera)
  frame.startNewFrame(bool(0))
  frame.waitOnFrame()
  frame.startNewFrame(bool(0))
  frame.waitOnFrame()
  te = time.time()-ts
  elapsed = elapsed+te
  if te < min:
    min = te
  if te > max:
    max = te
  #print(frameNum)
  #frame.saveFrame("benchmark_"+str(frameNum)+".png", 0)
frame.saveFrame("benchmark_final.png", 0)

print("TIMES min %f avg %f max %f [sec]" % (min, elapsed/numFrames, max))
mem = (resource.getrusage(resource.RUSAGE_SELF).ru_maxrss+resource.getrusage(resource.RUSAGE_CHILDREN).ru_maxrss)/1024.0
print("MAXMEM %d [MB]" % mem)


