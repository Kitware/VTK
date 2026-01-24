#!/usr/bin/env python
from vtkmodules.vtkCommonCore import (
    vtkMath,
    vtkPoints
)
from vtkmodules.vtkCommonDataModel import vtkPolyData
from vtkmodules.vtkCommonSystem import vtkTimerLog
from vtkmodules.vtkFiltersMeshing import vtkVoronoi2D
from vtkmodules.vtkRenderingCore import (
    vtkActor,
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

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

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
voronoi.SetGenerateCellScalarsToRandom()
voronoi.SetOutputTypeToDelaunay()

# Time execution
timer = vtkTimerLog()
timer.StartTimer()
voronoi.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Number of points processed: {0}".format(NPts))
print("   Time to generate Delaunay triangulation: {0}".format(time))
print("   Number of output points: {0}".format(voronoi.GetOutput(1).GetNumberOfPoints()))
print("   Number of output triangles: {0}".format(voronoi.GetOutput(1).GetNumberOfCells()))
print("   Number of threads used: {0}".format(voronoi.GetNumberOfThreads()))

mapper = vtkPolyDataMapper()
mapper.SetInputConnection(voronoi.GetOutputPort(1))
mapper.SetScalarRange(0,64)

actor = vtkActor()
actor.SetMapper(mapper)
actor.GetProperty().EdgeVisibilityOn()
actor.GetProperty().SetColor(1,0,0)

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(actor)
ren1.SetBackground(1,1,1)
renWin.SetSize(300,300)
renWin.Render()

iren.Initialize()
# --- end of script --
