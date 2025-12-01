#!/usr/bin/env python
from vtkmodules.vtkCommonDataModel import (
    vtkBoundingBox
)

from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Test various bounding box intersection methods with a sphere
#

# Define a bounding box
min = [-1.1,-1.1,-1.1]
max = [1.1,1.1,1.1]
bbox = vtkBoundingBox(min,max)

radius = 0.5

# Tests: box intersects sphere
center = [-2,0,0]
assert (not bbox.IntersectsSphere(min,max,center,radius*radius))
assert (not bbox.IntersectsSphere(center,radius))

center = [2,0,0]
assert (not bbox.IntersectsSphere(min,max,center,radius*radius))
assert (not bbox.IntersectsSphere(center,radius))

center = [0,-2,0]
assert (not bbox.IntersectsSphere(min,max,center,radius*radius))
assert (not bbox.IntersectsSphere(center,radius))

center = [0,2,0]
assert (not bbox.IntersectsSphere(min,max,center,radius*radius))
assert (not bbox.IntersectsSphere(center,radius))

center = [0,0,-2]
assert (not bbox.IntersectsSphere(min,max,center,radius*radius))
assert (not bbox.IntersectsSphere(center,radius))

center = [0,0,-2]
assert (not bbox.IntersectsSphere(min,max,center,radius*radius))
assert (not bbox.IntersectsSphere(center,radius))

center = [0,0,0]
assert (bbox.IntersectsSphere(min,max,center,radius*radius))
assert (bbox.IntersectsSphere(center,radius))

center = [-1.25,0,0]
assert (bbox.IntersectsSphere(min,max,center,radius*radius))
assert (bbox.IntersectsSphere(center,radius))

# Tests: box inside sphere
radius = 3
center = [0,0,0]
assert (bbox.InsideSphere(min,max,center,radius*radius))

center = [-10,0,0]
assert (not bbox.InsideSphere(min,max,center,radius*radius))

center = [-2,0,0]
assert (not bbox.InsideSphere(min,max,center,radius*radius))
