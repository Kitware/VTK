#!/usr/bin/env python
from vtkmodules.vtkCommonCore import (
    vtkMath,
    vtkPoints,
)
from vtkmodules.vtkCommonDataModel import vtkPolyData
from vtkmodules.vtkCommonSystem import vtkTimerLog
from vtkmodules.vtkFiltersCore import (
    vtkStaticCleanPolyData,
    vtkVoronoi2D,
)
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
#NPts = 1000000
MaxTileClips = NPts
PointsPerBucket = 2
GenerateFlower = 1
PointOfInterest = -1

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

ptMapper = vtkPointGaussianMapper()
ptMapper.SetInputData(profile)
ptMapper.EmissiveOff()
ptMapper.SetScaleFactor(0.0)

ptActor = vtkActor()
ptActor.SetMapper(ptMapper)
ptActor.GetProperty().SetColor(0,0,0)
ptActor.GetProperty().SetPointSize(2)

# Tessellate them
#
voronoi = vtkVoronoi2D()
voronoi.SetInputData(profile)
voronoi.SetGenerateScalarsToNone()
voronoi.SetGenerateScalarsToThreadIds()
voronoi.SetGenerateScalarsToPointIds()
voronoi.SetPointOfInterest(PointOfInterest)
voronoi.SetMaximumNumberOfTileClips(MaxTileClips)
voronoi.GetLocator().SetNumberOfPointsPerBucket(PointsPerBucket)
voronoi.SetGenerateVoronoiFlower(GenerateFlower)
voronoi.Update()

clean = vtkStaticCleanPolyData()
#clean = vtkCleanPolyData()
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

mapper = vtkPolyDataMapper()
mapper.SetInputConnection(clean.GetOutputPort())
mapper.SetScalarRange(0,NPts)
print("Scalar Range: {}".format(mapper.GetScalarRange()))

actor = vtkActor()
actor.SetMapper(mapper)
actor.GetProperty().SetColor(1,0,0)

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(actor)
ren1.AddActor(ptActor)

ren1.SetBackground(1,1,1)
renWin.SetSize(300,300)
renWin.Render()
cam1 = ren1.GetActiveCamera()

renWin.Render()
iren.Start()
# --- end of script --
