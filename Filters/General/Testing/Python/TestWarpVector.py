#!/usr/bin/env python
from vtkmodules.vtkCommonCore import (
    vtkDoubleArray,
    vtkMath,
)
from vtkmodules.vtkCommonSystem import vtkTimerLog
from vtkmodules.vtkFiltersGeneral import vtkWarpVector
from vtkmodules.vtkFiltersSources import vtkPlaneSource
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create a bunch of points and then transform them. Time the operations.

# Control resolution of test
res = 100
math = vtkMath()

# Create points and normals
plane = vtkPlaneSource()
plane.SetResolution(res,res)
plane.Update()

output = plane.GetOutput()

# Manually construct scalars
NPts = output.GetNumberOfPoints()
vectors = vtkDoubleArray()
vectors.SetNumberOfComponents(3)
vectors.SetNumberOfTuples(NPts)

for i in range(0,NPts):
    vectors.SetTuple3(i, math.Random(0,10), math.Random(0,10), math.Random(0,10))

output.GetPointData().SetVectors(vectors)

# Output some statistics
print("Number of points: {0}".format(NPts))

# Time the warping
warpF = vtkWarpVector()
warpF.SetInputData(output)
warpF.SetScaleFactor(2.5);

# For timing the various tests
timer = vtkTimerLog()

timer.StartTimer()
warpF.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Warp via vector: {0}".format(time))
