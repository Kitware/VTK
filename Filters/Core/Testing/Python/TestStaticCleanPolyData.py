#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
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
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# create some points and display them
#
math = vtk.vtkMath()
math.RandomSeed(31415)
points = vtk.vtkPoints()
i = 0
while i < NPts:
    points.InsertPoint(i,math.Random(0,1),math.Random(0,1),0.0)
    i = i + 1

profile = vtk.vtkPolyData()
profile.SetPoints(points)

ptMapper = vtk.vtkPointGaussianMapper()
ptMapper.SetInputData(profile)
ptMapper.EmissiveOff()
ptMapper.SetScaleFactor(0.0)

ptActor = vtk.vtkActor()
ptActor.SetMapper(ptMapper)
ptActor.GetProperty().SetColor(0,0,0)
ptActor.GetProperty().SetPointSize(2)

# Tessellate them
#
voronoi = vtk.vtkVoronoi2D()
voronoi.SetInputData(profile)
voronoi.SetGenerateScalarsToNone()
voronoi.SetGenerateScalarsToThreadIds()
voronoi.SetGenerateScalarsToPointIds()
voronoi.SetPointOfInterest(PointOfInterest)
voronoi.SetMaximumNumberOfTileClips(MaxTileClips)
voronoi.GetLocator().SetNumberOfPointsPerBucket(PointsPerBucket)
voronoi.SetGenerateVoronoiFlower(GenerateFlower)
voronoi.Update()

clean = vtk.vtkStaticCleanPolyData()
#clean = vtk.vtkCleanPolyData()
clean.SetInputConnection(voronoi.GetOutputPort())
clean.ToleranceIsAbsoluteOn()
clean.SetAbsoluteTolerance(0.00001)

# Time execution
timer = vtk.vtkTimerLog()
timer.StartTimer()
clean.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Number of points processed: {0}".format(NPts))
print("   Time to clean: {0}".format(time))
print("   #In pts: {0}".format(clean.GetInput().GetNumberOfPoints()))
print("   #Out pts: {0}".format(clean.GetOutput().GetNumberOfPoints()))

mapper = vtk.vtkPolyDataMapper()
mapper.SetInputConnection(clean.GetOutputPort())
mapper.SetScalarRange(0,NPts)
print("Scalar Range: {}".format(mapper.GetScalarRange()))

actor = vtk.vtkActor()
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
