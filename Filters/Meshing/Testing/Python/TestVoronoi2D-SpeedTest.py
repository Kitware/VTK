#!/usr/bin/env python
from vtkmodules.vtkCommonCore import (
    vtkMath,
    vtkPoints
)
from vtkmodules.vtkCommonDataModel import vtkPolyData
from vtkmodules.vtkCommonSystem import vtkTimerLog
from vtkmodules.vtkFiltersMeshing import vtkVoronoi2D

import sys

# Control problem size and set debugging parameters. For VTK
# testing (ctest), a default value is used. Otherwise, users can
# manually run the test with a specified number of points.
NPts = 1000
if len(sys.argv) > 1:
    try:
        NPts = int(sys.argv[1])
    except ValueError:
        NPts = 1000

PointsPerBucket = 1

# create some points and display them
#
math = vtkMath()
math.RandomSeed(31415)
points = vtkPoints()
i = 0
while i < NPts:
    points.InsertPoint(i,math.Random(0,1),math.Random(0,1),0.0)
    i = i + 1

profile = vtkPolyData()
profile.SetPoints(points)

# Tessellate them
#
voronoi = vtkVoronoi2D()
voronoi.SetInputData(profile)
voronoi.GetLocator().SetNumberOfPointsPerBucket(PointsPerBucket)
voronoi.SetGenerateCellScalarsToNone()
voronoi.SetOutputTypeToSpeedTest()
voronoi.ValidateOn()

# Time execution
timer = vtkTimerLog()
timer.StartTimer()
voronoi.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Number of points processed: {0}".format(NPts))
print("   Time to generate Voronoi tessellation: {0}".format(time))
print("   Number of threads used: {0}".format(voronoi.GetNumberOfThreadsUsed()))
print("   Max number of points/edges in any tile: {0}".format(voronoi.GetMaximumNumberOfPoints()))

# --- end of script --
