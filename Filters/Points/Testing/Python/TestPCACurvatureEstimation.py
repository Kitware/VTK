#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# create pipeline
#

# Create a cylinder
cyl = vtk.vtkCylinderSource()
cyl.SetCenter(-2,0,0)
cyl.SetRadius(0.02)
cyl.SetHeight(1.8)
cyl.SetResolution(24)

# Create a (thin) box implicit function
plane = vtk.vtkPlaneSource()
plane.SetOrigin(-1, -0.5, 0)
plane.SetPoint1(0.5, -0.5, 0)
plane.SetPoint2(-1, 0.5, 0)

# Create a sphere implicit function
sphere = vtk.vtkSphereSource()
sphere.SetCenter(2,0,0)
sphere.SetRadius(0.8)
sphere.SetThetaResolution(96)
sphere.SetPhiResolution(48)

# Boolean (union) these together
append = vtk.vtkAppendPolyData()
append.AddInputConnection(cyl.GetOutputPort())
append.AddInputConnection(plane.GetOutputPort())
append.AddInputConnection(sphere.GetOutputPort())

# Extract points along sphere surface
pts = vtk.vtkPolyDataPointSampler()
pts.SetInputConnection(append.GetOutputPort())
pts.SetDistance(0.01)
pts.Update()

# Now generate normals from resulting points
curv = vtk.vtkPCACurvatureEstimation()
curv.SetInputConnection(pts.GetOutputPort())
curv.SetSampleSize(20)

# Time execution
timer = vtk.vtkTimerLog()
timer.StartTimer()
curv.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Points processed: {0}".format(pts.GetOutput().GetNumberOfPoints()))
print("   Time to generate curvature: {0}".format(time))

# Break out the curvature into three separate arrays
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

iren.Start()
