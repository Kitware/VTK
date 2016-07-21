#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Parameters for testing
sze = 300

# Graphics stuff
ren0 = vtk.vtkRenderer()
ren0.SetViewport(0,0,0.5,1)
ren1 = vtk.vtkRenderer()
ren1.SetViewport(0.5,0,1,1)
renWin = vtk.vtkRenderWindow()
renWin.SetSize(2*sze+100,sze)
renWin.AddRenderer(ren0)
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Create pipeline, render simple object. We'll also color
# the sphere to generate color scalars.
sphere = vtk.vtkSphereSource()
sphere.SetCenter(0,0,0)
sphere.SetRadius(1)

ele = vtk.vtkElevationFilter()
ele.SetInputConnection(sphere.GetOutputPort())
ele.SetLowPoint(0,-1,0)
ele.SetHighPoint(0,1,0)

sphereMapper = vtk.vtkPolyDataMapper()
sphereMapper.SetInputConnection(ele.GetOutputPort())

sphereActor = vtk.vtkActor()
sphereActor.SetMapper(sphereMapper)

ren0.AddActor(sphereActor)
ren0.SetBackground(0,0,0)

iren.Initialize()
ren0.ResetCamera()
ren0.GetActiveCamera().SetClippingRange(6,9)
renWin.Render()

# Extract rendered geometry, convert to point cloud
# Grab just z-values
renSource = vtk.vtkRendererSource()
renSource.SetInput(ren0)
renSource.WholeWindowOff()
renSource.DepthValuesOnlyOn()
renSource.Update()

# Grab color values
renSource1 = vtk.vtkRendererSource()
renSource1.SetInput(ren0)
renSource1.WholeWindowOff()
renSource1.DepthValuesOff()
renSource1.DepthValuesInScalarsOff()
renSource1.Update()

# Note that the vtkPointGaussianMapper does not require vertex cells
pc = vtk.vtkDepthImageToPointCloud()
pc.SetInputConnection(0,renSource.GetOutputPort())
pc.SetInputConnection(1,renSource1.GetOutputPort())
pc.SetCamera(ren0.GetActiveCamera())
pc.CullNearPointsOn()
pc.CullFarPointsOn()
pc.ProduceVertexCellArrayOff()
print(pc)

timer = vtk.vtkTimerLog()
timer.StartTimer()
pc.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Generate point cloud: {0}".format(time))

pcMapper = vtk.vtkPointGaussianMapper()
pcMapper.SetInputConnection(pc.GetOutputPort())
pcMapper.EmissiveOff()
pcMapper.SetScaleFactor(0.0)

pcActor = vtk.vtkActor()
pcActor.SetMapper(pcMapper)

ren1.AddActor(pcActor)
ren1.SetBackground(0,0,0)
cam = ren1.GetActiveCamera()
cam.SetFocalPoint(0,0,0)
cam.SetPosition(1,1,1)
ren1.ResetCamera()

renWin.Render()
#iren.Start()
