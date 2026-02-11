#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

import sys

# Control problem size and set debugging parameters. For VTK
# testing (ctest), a default value is used. Otherwise, users can
# manually run the test with a specified number of points.
NPts = 1000
if len(sys.argv) > 1:
    try:
        NPts = int(sys.argv[1])
    except ValueError:
        NPts = NPts

PointsPerBucket = 1

# create some points and display them. Points have an associated
# point id (for debugging), and a region id (surface net).
#
math = vtk.vtkMath()
math.RandomSeed(31415)
points = vtk.vtkPoints()
points.SetNumberOfPoints(NPts)
ids = vtk.vtkIdTypeArray()
ids.SetName("Point Ids")
ids.SetNumberOfTuples(NPts)
regions = vtk.vtkIntArray()
regions.SetName("Region Ids")
regions.SetNumberOfTuples(NPts)
i = 0
while i < NPts:
    x = math.Random(0,2)
    y = math.Random(0,4)
    z = math.Random(0,6)
    points.SetPoint(i,x,y,z)
    ids.SetValue(i,i)
    r2 = (x*x + y*y + z*z)
    if r2 <= 1 :
        regions.SetValue(i,0)
    else:
        regions.SetValue(i,1)
    i = i + 1

profile = vtk.vtkUnstructuredGrid()
profile.SetPoints(points)
profile.GetPointData().AddArray(ids)
profile.GetPointData().AddArray(regions)

#
voronoi = vtk.vtkVoronoiFlower3D()
voronoi.SetInputData(profile)
voronoi.SetPadding(0.001)
voronoi.SetGenerateCellScalarsToNone()
voronoi.GetLocator().SetNumberOfPointsPerBucket(PointsPerBucket)
voronoi.SetOutputTypeToSpeedTest()
voronoi.SetInputArrayToProcess(0, 0, 0, vtk.vtkDataObject.FIELD_ASSOCIATION_POINTS, "Region Ids")
voronoi.SetBatchSize(1000)
voronoi.PassPointDataOn()
voronoi.MergePointsOff()
#voronoi.ValidateOn()

# Time execution
timer = vtk.vtkTimerLog()
timer.StartTimer()
voronoi.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Number of points processed: {0}".format(NPts))
print("   Time to generate Voronoi tessellation: {0}".format(time))
print("   Number of threads used: {0}".format(voronoi.GetNumberOfThreads()))
print("   Max number of points in any hull: {0}".format(voronoi.GetMaximumNumberOfPoints()))
print("   Max number of faces in any hull: {0}".format(voronoi.GetMaximumNumberOfFaces()))

# --- end of script --
