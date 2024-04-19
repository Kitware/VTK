#!/usr/bin/env python
from vtkmodules.vtkCommonCore import (
    vtkDoubleArray,
    vtkMath,
)
from vtkmodules.vtkCommonSystem import vtkTimerLog
from vtkmodules.vtkCommonTransforms import vtkTransform
from vtkmodules.vtkFiltersGeneral import vtkTransformPolyDataFilter
from vtkmodules.vtkFiltersSources import vtkPlaneSource
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create a bunch of points and then transform them. Time the operations.

# Control resolution of test
res = 10000
res = 750
math = vtkMath()

# Create points and normals
plane = vtkPlaneSource()
plane.SetResolution(res,res)
plane.Update()

output = plane.GetOutput()

# Manually construct vectors
NPts = output.GetNumberOfPoints()
vectors = vtkDoubleArray()
vectors.SetNumberOfComponents(3)
vectors.SetNumberOfTuples(NPts)

for i in range(0,NPts):
    vectors.SetTuple3(i, math.Random(-1,1), math.Random(-1,1),
                      math.Random(-1,1))

output.GetPointData().SetVectors(vectors)

# Output some statistics
print("Number of points: {0}".format(NPts))

# Time the transformation
xForm = vtkTransform()
xForm.Translate(1,2,3)
xForm.Scale(1,2,3)
xForm.RotateX(10)
xForm.RotateY(20)
xForm.RotateZ(30)

xFormF = vtkTransformPolyDataFilter()
xFormF.SetInputData(output)
xFormF.SetTransform(xForm)

# For timing the various tests
timer = vtkTimerLog()

timer.StartTimer()
xFormF.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Transform: {0}".format(time))
