#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Control problem size and set debugging parameters
NPts = 1000
#NPts = 1000000
MaxTileClips = 10000
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

#points.InsertPoint(0,0,0,0.0)
#points.InsertPoint(1,0,-2,0.0)
#points.InsertPoint(2,2,0,0.0)
#points.InsertPoint(3,0,4,0.0)
#points.InsertPoint(4,-6,0,0.0)

#points.InsertPoint(0,0,0,0.0)
#points.InsertPoint(1,0,-2,0.0)
#points.InsertPoint(2,2,0,0.0)
#points.InsertPoint(3,0,2,0.0)
#points.InsertPoint(4,-2,0,0.0)

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

# Time execution
timer = vtk.vtkTimerLog()
timer.StartTimer()
voronoi.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Number of points processed: {0}".format(NPts))
print("   Time to generate Voronoi tessellation: {0}".format(time))
print("   Number of threads used: {0}".format(voronoi.GetNumberOfThreadsUsed()))

mapper = vtk.vtkPolyDataMapper()
mapper.SetInputConnection(voronoi.GetOutputPort())
if voronoi.GetGenerateScalars() == 1:
    mapper.SetScalarRange(0,NPts)
elif voronoi.GetGenerateScalars() == 2:
    mapper.SetScalarRange(0,voronoi.GetNumberOfThreadsUsed())
print("Scalar Range: {}".format(mapper.GetScalarRange()))

actor = vtk.vtkActor()
actor.SetMapper(mapper)
actor.GetProperty().SetColor(1,0,0)

# Debug code
sphere = vtk.vtkSphereSource()
sphere.SetRadius(0.05)
sphere.SetThetaResolution(16)
sphere.SetPhiResolution(8)
if PointOfInterest >= 0:
    sphere.SetCenter(points.GetPoint(PointOfInterest))

sphereMapper = vtk.vtkPolyDataMapper()
sphereMapper.SetInputConnection(sphere.GetOutputPort())

sphereActor = vtk.vtkActor()
sphereActor.SetMapper(sphereMapper)
sphereActor.GetProperty().SetColor(0,0,0)

# Voronoi flower
fMapper = vtk.vtkPointGaussianMapper()
fMapper.SetInputConnection(voronoi.GetOutputPort(1))
fMapper.EmissiveOff()
fMapper.SetScaleFactor(0.0)

fActor = vtk.vtkActor()
fActor.SetMapper(fMapper)
fActor.GetProperty().SetColor(0,0,1)

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(actor)
ren1.AddActor(ptActor)
if PointOfInterest >= 0:
    ren1.AddActor(sphereActor)
    if GenerateFlower > 0:
         ren1.AddActor(fActor)
         ren1.RemoveActor(ptActor)

ren1.SetBackground(1,1,1)
renWin.SetSize(300,300)
renWin.Render()
cam1 = ren1.GetActiveCamera()
if PointOfInterest >= 0:
    xyz = []
    xyz = points.GetPoint(PointOfInterest)
    cam1.SetFocalPoint(xyz)
    cam1.SetPosition(xyz[0],xyz[1],xyz[2]+1)
    renWin.Render()
    ren1.ResetCameraClippingRange()
    cam1.Zoom(1.5)

# added these unused default arguments so that the prototype
# matches as required in python.
def reportPointId (a=0,b=0,__vtk__temp0=0,__vtk__temp1=0):
    print("Point Id: {}".format(picker.GetPointId()))

picker = vtk.vtkPointPicker()
picker.AddObserver("EndPickEvent",reportPointId)
picker.PickFromListOn()
picker.AddPickList(ptActor)
iren.SetPicker(picker)

renWin.Render()
iren.Start()
# --- end of script --
