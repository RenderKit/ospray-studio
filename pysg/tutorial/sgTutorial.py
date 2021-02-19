#Copyright 2021 Intel Corporation
#SPDX-License-Identifier: Apache-2.0

import sys, numpy
import pysg as sg
from pysg import Any, vec3f

sg.init(sys.argv)

# specific rkcommon::math types needed for SG
pos = Any(vec3f(20.0, 20.0, 89.28))
dir = Any(vec3f(0.0, 0.0, -1.0))
up = Any(vec3f(0.0, 1.0, 0.0))

W = 1024
H = 768

aspect = sg.Any(W/H)

frame = sg.Frame()
world = frame.child("world")

cam = frame.child("camera")
cam.createChild("aspect", "float", aspect)
cam.createChild("position", "vec3f", pos)
cam.createChild("direction", "vec3f", dir)
cam.createChild("up", "vec3f", up)

geom = world.createChild("spheres", "geometry_spheres")
geom.createChildData("sphere.position", vec3f(1.0))
geom.createChild("radius", "float", Any(20.0))

world.render()

frame.startNewFrame(bool(0))
frame.waitOnFrame()
frame.saveFrame("testFrame.jpg", 0)
