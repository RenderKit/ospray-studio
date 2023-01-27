#Copyright 2021 Intel Corporation
#SPDX-License-Identifier: Apache-2.0

"""
An example of using python to benchmark studio rendering of vtk data files.
Example use:
 python3 studioDistbench1.py -numFrames 200 -colors 2 0 0 0 1 1 1 filename.filetype
 filename should be in the format filename$(RankID).filetype

To pack data set blocks into a cube use the -pack flag as the following
mpirun -np 3 python3 studioDistbench1-pack.py -numFrames 200 -colors 2 1 0 0 0 1 0 -pack filename.pvd

"""

import sys, numpy, math, time, atexit, resource
from mpi4py import MPI
import pysg as sg
from pysg import Any, vec3f, Data, FileName, Importer, MaterialRegistry, ArcballCamera, vec2i, box3f, vec2f, vec3i, affine3f

args = sg.init(sys.argv)

mpiRank = 0
mpiWorldSize = 1
comm = MPI.COMM_WORLD
mpiRank = comm.Get_rank()
mpiWorldSize = comm.Get_size()

sg.assignMPI(mpiRank, mpiWorldSize)

#optional command line arguments
scalarRange = None
colors = None
opacities = None
numTriangles = 1000000
dim = 200
W = 1024
H = 768
numFrames = 100
packData = False 
numDataFiles = 0
imgPath = ""
  
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
  if a == "-numTriangles":
     numTriangles = int(args[idx+1])
  if a == "-dim":
     dim = int(args[idx+1])
  if a == "-res":
    W = int(args[idx+1])
    H = int(args[idx+2])
  if a == "-numFrames":
    numFrames = int(args[idx+1])
  if a == "-pack":
    packData = True
  if a == "-imgPath":
    imgPath = args[idx+1] + "/"

filename = args[len(args)-1]
volumename = ""
path = ""
filetype = ""

if filename != "wavelet" and filename != "slices":
  sg.loadPlugin("vtk")
  volumename = filename.split("/")[-1]
  path = filename[:len(filename) - len(volumename)]
  extps = volumename.rfind(".")
  filetype = volumename[extps:]
  volumename = volumename[0:extps]

myFilename = ""
if filetype != ".pvd":
  myFilename = path + volumename + str(mpiRank) + filetype
else:
  myFilename = path + volumename + filetype

window_size = vec2i(W, H)
aspect = Any(float(W) / H)
 
frame = sg.Frame()
frame.immediatelyWait = True
frame.createChild("windowSize", "vec2i", Any(window_size))
  
renderer = sg.Renderer("mpiRaycast")
frame.createChildAs("renderer", "renderer_mpiRaycast")
  
world = frame.child("world")
  
lightsMan = frame.child("lights")
baseMaterialRegistry = frame.child("baseMaterialRegistry")

nTrianglesGetter = None
if filename == "slices":
  ws = world.createChildAs("generator", "generator_wavelet_slices")
  params = ws.child("parameters")
  params.createChild("requestedTriangles", "long", Any(numTriangles))
  params.createChild("numSlices", "int", Any(200))
  params.createChild("thresholdLow", "float", Any(-0.00005))
  params.createChild("thresholdHigh", "float", Any(0.00005))
  nTrianglesGetter = params.child("actualTriangles")
elif filename == "wavelet":
  ws = world.createChildAs("generator", "generator_wavelet")
  params = ws.child("parameters")
  params.createChild("dimensions", "vec3i", Any(vec3i(dim)))
else:
  importer = sg.getImporter(world, FileName(myFilename))
  importer.setLightsManager(lightsMan)
  importer.setMaterialRegistry(baseMaterialRegistry)
  importer.importScene()
  print("loading "+myFilename)

  if filetype == ".vti" or filetype == ".vtu":
    #it is a good idea to standardadize LUT range to keep picture consistent
    #traverse scenegraph to the place we need to hange LUT on
    importer = world.child(volumename+ str(mpiRank) +"_importer")
    transform = importer.child("xfm")
    tfOrig = transform.child("transferFunction")
    if scalarRange is not None:
      tfOrig.remove("value")
      tfOrig.createChild("value", "range1f", Any(scalarRange))
    if colors is not None:
      npColors = numpy.array(colors,dtype=numpy.float32)
      tfOrig.remove("color")
      tfOrig.createChildData("color", Data(npColors))
    if opacities is not None:
      npOpacities = numpy.array(opacities,dtype=numpy.float32)
      tfOrig.remove("opacity")
      tfOrig.createChildData("opacity", Data(npOpacities))
  else:
    if scalarRange is not None:
      importer.remove("value")
      importer.createChild("value", "range1f", Any(scalarRange))
    if colors is not None:
      npColors = numpy.array(colors,dtype=numpy.float32)
      importer.remove("color")
      importer.createChildData("color", Data(npColors))
    if opacities is not None:
      npOpacities = numpy.array(opacities,dtype=numpy.float32)
      importer.remove("opacity")
      importer.createChildData("opacity", Data(npOpacities))
  if packData is True:
    world.render()
    filenames = []
    with open(filename) as f:
      lines = f.readlines()
      for line in lines:
        if "DataSet" in line:
          currLine = line.split("file=")[1].split("/")
          currLine = currLine[len(currLine)-2].split('"')
          currFileName = currLine[len(currLine)-2]
          filenames.append(currFileName)
     
    numDataFiles = len(filenames) 
    cubed = int(pow(numDataFiles, 1.0/3.0) + 0.99)
    fid = 0
    for i in range(0, cubed):
      for j in range(0, cubed):
        for k in range(0, cubed):
          if fid < numDataFiles and int(fid % mpiWorldSize)  == mpiRank:
              fileName = filenames[fid]
              localBounds = world.bounds()
              dims = vec3f(localBounds.upper.x - localBounds.lower.x, localBounds.upper.y - localBounds.lower.y, localBounds.upper.z - localBounds.lower.z)
              maxes = [dims.x, dims.y, dims.z]
              localBounds.lower = vec3f(maxes[0]*k,maxes[1]*j,maxes[2]*i)
              localBounds.upper.x = localBounds.lower.x + dims.x
              localBounds.upper.y = localBounds.lower.y + dims.y
              localBounds.upper.z = localBounds.lower.z + dims.z

              if fileName.rfind("vti") == len(fileName)-3:
                subimporter = importer.child(path + "/" + fileName)
                volNode = subimporter.child("transferFunction").child("vti")
                volNode.createChild("gridOrigin", "vec3f", Any(localBounds.lower) )
                volNode.createChild("mpiRegion", "box3f", Any(localBounds))
              else:
                subimporter = importer.child(path + "/" + fileName)
                subimporter.setValue(affine3f.translate(localBounds.lower), True)
                geomNode = subimporter.child("vtk_polydata")
                bds = geomNode.child("vtkbounds").valueAsBox3f()
                bds.lower.x = bds.lower.x + localBounds.lower.x
                bds.lower.y = bds.lower.y + localBounds.lower.y
                bds.lower.z = bds.lower.z + localBounds.lower.z
                bds.upper.x = bds.upper.x + localBounds.lower.x
                bds.upper.y = bds.upper.y + localBounds.lower.y
                bds.upper.z = bds.upper.z + localBounds.lower.z
                geomNode.createChild("mpiRegion","box3f", Any(bds))

          fid = fid + 1

world.render()

if nTrianglesGetter is not None:
  print("rank", mpiRank, "numTriangles", nTrianglesGetter.valueAsInt())

bounds = world.bounds()
'''
#this is how to get bounds information
l = bounds.lower
u = bounds.upper
print(l.x)
print(l.y)
print(l.z)
print(u.x)
print(u.y)
print(u.z)
'''

arcballCamera = ArcballCamera(bounds, window_size)
cam = frame.child("camera")
cam.createChild("aspect", "float", aspect)

sg.updateCamera(cam, arcballCamera)
  
# First frame will be "navigation" resolution.
# Render again for full sized frame.
frame.startNewFrame()
frame.startNewFrame()
frame.waitOnFrame()
outputName = imgPath + "benchmarkDist_initial" + str(mpiRank) + ".png"

if mpiRank == 0:
    frame.saveFrame(outputName, 0)
elapsed = 0.0
min = 1000000.0
max = -1.0
for frameNum in range(0,numFrames):
  ts = time.time()
  arcballCamera.rotate(vec2f(-2.0/numFrames, 0.0),vec2f(2.0/numFrames, 0.0))
  sg.updateCamera(cam, arcballCamera)
  frame.startNewFrame()
  frame.waitOnFrame()
  frame.startNewFrame()
  frame.waitOnFrame()
  te = time.time()-ts
  elapsed = elapsed+te
  if te < min:
    min = te
  if te > max:
    max = te
  if mpiRank == 0:
      frame.saveFrame(imgPath + "benchmarkDist_"+str(frameNum)+".png", 0)

outputName = imgPath + "benchmarkDist_final" + str(mpiRank) + ".png"
if mpiRank == 0:
    frame.saveFrame(outputName, 0)

print("TIMES min %f avg %f max %f [sec]" % (min, elapsed/numFrames, max))
mem = (resource.getrusage(resource.RUSAGE_SELF).ru_maxrss+resource.getrusage(resource.RUSAGE_CHILDREN).ru_maxrss)/1024.0
print("MAXMEM %d [MB]" % mem)

