#!/usr/bin/env python
from vtkmodules.vtkCommonCore import (
    vtkIdTypeArray,
    vtkMath,
)
from vtkmodules.vtkCommonDataModel import (
    vtkPointLocator,
    vtkStaticPointLocator,
)
from vtkmodules.vtkCommonSystem import vtkTimerLog
from vtkmodules.vtkFiltersSources import vtkPointSource
from vtkmodules.test import Testing

try:
    import numpy as np
except ImportError:
    print("This test requires numpy")
    Testing.skip()

from vtkmodules.util.numpy_support import vtk_to_numpy

# Test the vtkStaticPointLocator::MergePoints() method.

# create a test dataset
#
math = vtkMath()

# Note: the bigger the data the better vtkStaticPointLocator performs (as
# compared to vtkPointLocator).
#testSize = "large"
#testSize = "medium"
testSize = "small"

if testSize == "large":
    numPts = 20000000
elif testSize == "medium":
    numPts = 2000000
else:
    numPts = 20000

# Create an initial set of points and associated dataset
rpts = vtkPointSource()
rpts.SetNumberOfPoints(numPts)
rpts.SetDistributionToUniform()
rpts.SetRadius(1)
rpts.Update()
polydata = rpts.GetOutput()

# Print initial statistics
print("Processing NumPts: {0}".format(numPts))

timer = vtkTimerLog()

# Create the locator.
sclean = vtkStaticPointLocator()
sclean.SetDataSet(polydata)

timer.StartTimer()
sclean.BuildLocator()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Build Static Point Locator (threaded): {0}".format(time))
print("Number of Divisions: ", sclean.GetDivisions())

# Create a comparison point locator.
clean = vtkPointLocator()
clean.SetDataSet(polydata)

timer.StartTimer()
clean.BuildLocator()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Build Point Locator (serial): {0}".format(time))
print("Number of Divisions: ", clean.GetDivisions())

# The merge map indicates how input points are merged to output points.
mergeMapArray = vtkIdTypeArray()
mergeMapArray.SetNumberOfTuples(polydata.GetNumberOfPoints())
mergeMap = vtk_to_numpy(mergeMapArray)

# Test the point merging process - 0 tolerance. The traversal
# ordering mode is irrelevant.
timer.StartTimer()
sclean.MergePoints(0.0, mergeMap)
timer.StopTimer()
time = timer.GetElapsedTime()
print("\nStatic Point Locator: Merge Points (zero tolerance): {0}".format(time))

# POINT_ORDERING - serial traversal
timer.StartTimer()
sclean.SetTraversalOrder(0)
sclean.MergePoints(0.01, mergeMap)
timer.StopTimer()
time = timer.GetElapsedTime()
print("Static Point Locator: Merge Points (0.01 tolerance, serial): {0}".format(time))

# BIN_ORDERING - threaded traversal
timer.StartTimer()
sclean.SetTraversalOrder(1)
sclean.MergePoints(0.01, mergeMap)
timer.StopTimer()
time = timer.GetElapsedTime()
print("Static Point Locator: Merge Points (0.01 tolerance, threaded): {0}".format(time))
