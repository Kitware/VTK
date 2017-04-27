#!/usr/bin/env python
import vtk
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
#renWin.SetSize(2*sze,sze)
renWin.AddRenderer(ren0)
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Create pipeline, render simple object. We'll also color
# the plane.
plane = vtk.vtkPlaneSource()
plane.SetOrigin(0,0,0)
plane.SetPoint1(2,0,0)
plane.SetPoint2(0,1,0)

ele = vtk.vtkElevationFilter()
ele.SetInputConnection(plane.GetOutputPort())
ele.SetLowPoint(0,0,0)
ele.SetHighPoint(0,1,0)

planeMapper = vtk.vtkPolyDataMapper()
planeMapper.SetInputConnection(ele.GetOutputPort())

planeActor = vtk.vtkActor()
planeActor.SetMapper(planeMapper)

ren0.AddActor(planeActor)
ren0.SetBackground(1,1,1)

iren.Initialize()
renWin.Render()

# Extract rendered geometry, convert to point cloud
renSource = vtk.vtkRendererSource()
renSource.SetInput(ren0)
renSource.WholeWindowOff()
renSource.DepthValuesOn()
renSource.Update()

pc = vtk.vtkDepthImageToPointCloud()
pc.SetInputConnection(renSource.GetOutputPort())
pc.SetCamera(ren0.GetActiveCamera())
pc.CullFarPointsOff()

timer = vtk.vtkTimerLog()
timer.StartTimer()
pc.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Generate point cloud: {0}".format(time))

pcMapper = vtk.vtkPolyDataMapper()
pcMapper.SetInputConnection(pc.GetOutputPort())

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
