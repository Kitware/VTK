#!/usr/bin/env python

import math
from vtkmodules.vtkCommonCore import (
    vtkPoints,
    vtkFloatArray
)
from vtkmodules.vtkCommonDataModel import vtkPolyData
from vtkmodules.vtkFiltersCore import (
    vtkElevationFilter,
    vtkThresholdScalars
)
from vtkmodules.vtkFiltersSources import vtkLineSource

import vtkmodules.test.Testing
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create the data -- a line with added scalars
#
line = vtkLineSource()
line.SetResolution(10)
line.Update()

pd = line.GetOutput()
s = vtkFloatArray()
s.SetNumberOfTuples(pd.GetNumberOfPoints())
s.SetValue(0,-10)
s.SetValue(1,-5)
s.SetValue(2,-4)
s.SetValue(3,-3)
s.SetValue(4,0)
s.SetValue(5,3)
s.SetValue(6,4)
s.SetValue(7,5)
s.SetValue(8,10)
s.SetValue(9,12)
s.SetValue(10,15)
pd.GetPointData().SetScalars(s)

# Define some intervals
threshold = vtkThresholdScalars()
threshold.SetInputData(pd)
threshold.AddInterval(-5,-3,0);
threshold.AddInterval(-2,2,1);
intId = threshold.AddInterval(5,3,2); #handle incorrect interval specification
threshold.AddInterval(8,12,3);
threshold.SetBackgroundLabel(-250)
threshold.Update()

# Test various scalar values
scalars = threshold.GetOutput().GetPointData().GetScalars()
#for idx in range(0,pd.GetNumberOfPoints()):
#    print(s.GetValue(idx),scalars.GetValue(idx))

assert(scalars.GetValue(0) == -250)
assert(scalars.GetValue(1) == 0)
assert(scalars.GetValue(2) == 0)
assert(scalars.GetValue(3) == -250)
assert(scalars.GetValue(4) == 1)
assert(scalars.GetValue(5) == 2)
assert(scalars.GetValue(6) == 2)
assert(scalars.GetValue(7) == -250)
assert(scalars.GetValue(8) == 3)
assert(scalars.GetValue(9) == -250)
assert(scalars.GetValue(10) == -250)

threshold.RemoveInterval(intId)
threshold.Update()
scalars = threshold.GetOutput().GetPointData().GetScalars()
assert(scalars.GetValue(0) == -250)
assert(scalars.GetValue(1) == 0)
assert(scalars.GetValue(2) == 0)
assert(scalars.GetValue(3) == -250)
assert(scalars.GetValue(4) == 1)
assert(scalars.GetValue(5) == -250)
assert(scalars.GetValue(6) == -250)
assert(scalars.GetValue(7) == -250)
assert(scalars.GetValue(8) == 3)
assert(scalars.GetValue(9) == -250)
assert(scalars.GetValue(10) == -250)
