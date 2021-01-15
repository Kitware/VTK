#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create a bunch of points and then transform them. Time the operations.

# Control resolution of test
res = 100
math = vtk.vtkMath()

# Create points and normals
plane = vtk.vtkPlaneSource()
plane.SetResolution(res,res)
plane.Update()

output = plane.GetOutput()

# Manually construct scalars
NPts = output.GetNumberOfPoints()
vectors = vtk.vtkDoubleArray()
vectors.SetNumberOfComponents(3)
vectors.SetNumberOfTuples(NPts)

for i in range(0,NPts):
    vectors.SetTuple3(i, math.Random(0,10), math.Random(0,10), math.Random(0,10))

output.GetPointData().SetVectors(vectors)

# Output some statistics
print("Number of points: {0}".format(NPts))

# Time the warping
warpF = vtk.vtkWarpVector()
warpF.SetInputData(output)
warpF.SetScaleFactor(2.5);

# For timing the various tests
timer = vtk.vtkTimerLog()

timer.StartTimer()
warpF.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Warp via vector: {0}".format(time))
