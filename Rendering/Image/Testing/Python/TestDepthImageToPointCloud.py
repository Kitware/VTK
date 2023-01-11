#!/usr/bin/env python
from vtkmodules.vtkCommonSystem import vtkTimerLog
from vtkmodules.vtkFiltersCore import vtkElevationFilter
from vtkmodules.vtkFiltersSources import vtkPlaneSource
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
    vtkRendererSource,
)
from vtkmodules.vtkRenderingImage import vtkDepthImageToPointCloud
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Parameters for testing
sze = 300

# Graphics stuff
ren0 = vtkRenderer()
ren0.SetViewport(0,0,0.5,1)
ren1 = vtkRenderer()
ren1.SetViewport(0.5,0,1,1)
renWin = vtkRenderWindow()
renWin.SetSize(2*sze+100,sze)
#renWin.SetSize(2*sze,sze)
renWin.AddRenderer(ren0)
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Create pipeline, render simple object. We'll also color
# the plane.
plane = vtkPlaneSource()
plane.SetOrigin(0,0,0)
plane.SetPoint1(2,0,0)
plane.SetPoint2(0,1,0)

ele = vtkElevationFilter()
ele.SetInputConnection(plane.GetOutputPort())
ele.SetLowPoint(0,0,0)
ele.SetHighPoint(0,1,0)

planeMapper = vtkPolyDataMapper()
planeMapper.SetInputConnection(ele.GetOutputPort())

planeActor = vtkActor()
planeActor.SetMapper(planeMapper)

ren0.AddActor(planeActor)
ren0.SetBackground(1,1,1)

iren.Initialize()
renWin.Render()

# Extract rendered geometry, convert to point cloud
renSource = vtkRendererSource()
renSource.SetInput(ren0)
renSource.WholeWindowOff()
renSource.DepthValuesOn()
renSource.Update()

pc = vtkDepthImageToPointCloud()
pc.SetInputConnection(renSource.GetOutputPort())
pc.SetCamera(ren0.GetActiveCamera())
pc.CullFarPointsOff()

timer = vtkTimerLog()
timer.StartTimer()
pc.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Generate point cloud: {0}".format(time))

pcMapper = vtkPolyDataMapper()
pcMapper.SetInputConnection(pc.GetOutputPort())

pcActor = vtkActor()
pcActor.SetMapper(pcMapper)

ren1.AddActor(pcActor)
ren1.SetBackground(0,0,0)
cam = ren1.GetActiveCamera()
cam.SetFocalPoint(0,0,0)
cam.SetPosition(1,1,1)
ren1.ResetCamera()

renWin.Render()
#iren.Start()
