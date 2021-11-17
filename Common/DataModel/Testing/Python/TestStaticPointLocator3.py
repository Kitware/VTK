#!/usr/bin/env python
import vtk
from vtk.test import Testing

try:
    import numpy as np
except ImportError:
    print("Numpy (http://numpy.scipy.org) not found.")
    print("This test requires numpy")
    Testing.skip()

from vtk.util.numpy_support import vtk_to_numpy

# Test the vtkStaticPointLocator::MergePoints() method.

# create a test dataset
#
math = vtk.vtkMath()

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
rpts = vtk.vtkPointSource()
rpts.SetNumberOfPoints(numPts)
rpts.SetDistributionToUniform()
rpts.SetRadius(1)
rpts.Update()
polydata = rpts.GetOutput()

# Print initial statistics
print("Processing NumPts: {0}".format(numPts))

timer = vtk.vtkTimerLog()

# Create the locator.
sclean = vtk.vtkStaticPointLocator()
sclean.SetDataSet(polydata)

timer.StartTimer()
sclean.BuildLocator()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Build Static Point Locator (threaded): {0}".format(time))
print("Number of Divisions: ", sclean.GetDivisions())

# Create a comparison point locator.
clean = vtk.vtkPointLocator()
clean.SetDataSet(polydata)

timer.StartTimer()
clean.BuildLocator()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Build Point Locator (serial): {0}".format(time))
print("Number of Divisions: ", clean.GetDivisions())

# The merge map indicates how input points are merged to output points.
mergeMapArray = vtk.vtkIdTypeArray()
mergeMapArray.SetNumberOfTuples(polydata.GetNumberOfPoints())
mergeMap = vtk_to_numpy(mergeMapArray)

# Test the point merging process - 0 tolerance. The traversal
# ordering mode is irrelavant.
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
