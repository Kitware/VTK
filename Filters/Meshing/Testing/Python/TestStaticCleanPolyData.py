#!/usr/bin/env python
from vtkmodules.vtkCommonCore import (
    vtkMath,
    vtkPoints,
)
from vtkmodules.vtkCommonDataModel import vtkPolyData
from vtkmodules.vtkCommonSystem import vtkTimerLog
from vtkmodules.vtkFiltersCore import vtkStaticCleanPolyData
from vtkmodules.vtkFiltersMeshing import vtkVoronoiFlower2D
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkPointGaussianMapper,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Control problem size and set debugging parameters
NPts = 1000
PointsPerBucket = 2
GenerateFlower = 1
PointOfInterest = -1

# create some random points
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
voronoi = vtkVoronoiFlower2D()
voronoi.SetInputData(profile)
voronoi.SetGenerateCellScalarsToPointIds()
voronoi.SetPointOfInterest(PointOfInterest)
voronoi.GetLocator().SetNumberOfPointsPerBucket(PointsPerBucket)
voronoi.SetGenerateVoronoiFlower(GenerateFlower)
voronoi.Update()

# The absolute tolerance has been carefully chosen to clean one point.
clean = vtkStaticCleanPolyData()
clean.SetInputConnection(voronoi.GetOutputPort())
clean.ToleranceIsAbsoluteOn()
clean.SetAbsoluteTolerance(0.00001)

# Time execution
timer = vtkTimerLog()
timer.StartTimer()
clean.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Number of points processed: {0}".format(NPts))
print("   Time to clean: {0}".format(time))
print("   #In pts: {0}".format(clean.GetInput().GetNumberOfPoints()))
print("   #Out pts: {0}".format(clean.GetOutput().GetNumberOfPoints()))

numInPts = clean.GetInput().GetNumberOfPoints()
numOutPts = clean.GetOutput().GetNumberOfPoints()
assert(numInPts == numOutPts+1)

# --- end of script --
