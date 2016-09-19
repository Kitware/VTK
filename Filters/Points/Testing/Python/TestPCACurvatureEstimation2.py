#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Interpolate onto a volume

# Parameters for debugging
NPts = 1000000
math = vtk.vtkMath()
math.RandomSeed(31415)

# create pipeline
#
points = vtk.vtkBoundedPointSource()
points.SetNumberOfPoints(NPts)
points.SetBounds(-3,3, -1,1, -1,1)
points.ProduceRandomScalarsOn()
points.ProduceCellOutputOff()
points.Update()

# Create a cylinder
cyl = vtk.vtkCylinder()
cyl.SetCenter(-2,0,0)
cyl.SetRadius(0.02)

# Create a (thin) box implicit function
box = vtk.vtkBox()
box.SetBounds(-1,0.5, -0.5,0.5, -0.0005, 0.0005)

# Create a sphere implicit function
sphere = vtk.vtkSphere()
sphere.SetCenter(2,0,0)
sphere.SetRadius(0.8)

# Boolean (union) these together
imp = vtk.vtkImplicitBoolean()
imp.SetOperationTypeToUnion()
imp.AddFunction(cyl)
imp.AddFunction(box)
imp.AddFunction(sphere)

# Extract points along sphere surface
extract = vtk.vtkFitImplicitFunction()
extract.SetInputConnection(points.GetOutputPort())
extract.SetImplicitFunction(imp)
extract.SetThreshold(0.0005)
extract.Update()

# Now generate normals from resulting points
curv = vtk.vtkPCACurvatureEstimation()
curv.SetInputConnection(extract.GetOutputPort())
curv.SetSampleSize(6)

# Time execution
timer = vtk.vtkTimerLog()
timer.StartTimer()
curv.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Points processed: {0}".format(NPts))
print("   Time to generate curvature: {0}".format(time))

# Break out the curvature into thress separate arrays
assign = vtk.vtkAssignAttribute()
assign.SetInputConnection(curv.GetOutputPort())
assign.Assign("PCACurvature", "VECTORS", "POINT_DATA")

extract = vtk.vtkExtractVectorComponents()
extract.SetInputConnection(assign.GetOutputPort())
extract.Update()
print(extract.GetOutput(0).GetScalarRange())
print(extract.GetOutput(1).GetScalarRange())
print(extract.GetOutput(2).GetScalarRange())

# Three different outputs for different curvatures
subMapper = vtk.vtkPointGaussianMapper()
subMapper.SetInputConnection(extract.GetOutputPort(0))
subMapper.EmissiveOff()
subMapper.SetScaleFactor(0.0)

subActor = vtk.vtkActor()
subActor.SetMapper(subMapper)
subActor.AddPosition(0,2.25,0)

sub1Mapper = vtk.vtkPointGaussianMapper()
sub1Mapper.SetInputConnection(extract.GetOutputPort(1))
sub1Mapper.EmissiveOff()
sub1Mapper.SetScaleFactor(0.0)

sub1Actor = vtk.vtkActor()
sub1Actor.SetMapper(sub1Mapper)

sub2Mapper = vtk.vtkPointGaussianMapper()
sub2Mapper.SetInputConnection(extract.GetOutputPort(2))
sub2Mapper.EmissiveOff()
sub2Mapper.SetScaleFactor(0.0)

sub2Actor = vtk.vtkActor()
sub2Actor.SetMapper(sub2Mapper)
sub2Actor.AddPosition(0,-2.25,0)

# Create the RenderWindow, Renderer and both Actors
#
ren0 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren0)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren0.AddActor(subActor)
ren0.AddActor(sub1Actor)
ren0.AddActor(sub2Actor)
ren0.SetBackground(0.1, 0.2, 0.4)

renWin.SetSize(250,250)

cam = ren0.GetActiveCamera()
cam.SetFocalPoint(0,0,-1)
cam.SetPosition(0,0,0)
ren0.ResetCamera()

iren.Initialize()

# render the image
#
renWin.Render()

#iren.Start()
