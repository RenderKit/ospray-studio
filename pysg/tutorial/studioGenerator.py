#Copyright 2021 Intel Corporation
#SPDX-License-Identifier: Apache-2.0

"""
An example of using python to run the wavelet generator.
Example use:
  mpirun -np 2 python3 studioGenerator.py 
"""

import sys, numpy, math, time, atexit, resource
from mpi4py import MPI
import pysg as sg
from pysg import Any, vec3f, Data, FileName, Importer, MaterialRegistry, ArcballCamera, vec2i, box3f, vec2f

args = sg.init(sys.argv)


mpiRank = 0
mpiWorldSize = 1
comm = MPI.COMM_WORLD
mpiRank = comm.Get_rank()
mpiWorldSize = comm.Get_size()

#optional command line arguments
W = 1024
H = 768
numFrames = 100

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

world.createChildAs("generator", "generator_wavelet")  
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
outputName = "wavelet.png"

if mpiRank == 0:
    frame.saveFrame(outputName, 0)

