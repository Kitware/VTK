#!/usr/bin/env python
from vtkmodules.vtkCommonDataModel import (
    vtkPlane,
    vtkPlaneCollection,
)
from vtkmodules.vtkCommonSystem import vtkTimerLog
from vtkmodules.vtkFiltersCore import (
    vtkClipPolyData,
    vtkPolyDataPlaneClipper,
)
from vtkmodules.vtkFiltersGeneral import vtkClipClosedSurface
from vtkmodules.vtkFiltersSources import vtkSphereSource
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
from vtkmodules.test import Testing
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Control test size
res = 1024

# Create the RenderWindow, Renderers and both Actors
ren0 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren0)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Create a synthetic sphere
sphere = vtkSphereSource()
sphere.SetCenter(0.0, 0.0, 0.0)
sphere.SetRadius(0.25)
sphere.SetThetaResolution(2*res)
sphere.SetPhiResolution(res)
sphere.Update()
print("Processing: ", sphere.GetOutput().GetNumberOfCells(), " triangles")

# The cut plane
plane = vtkPlane()
plane.SetOrigin(0, 0, 0)
plane.SetNormal(-1, -1, -1)
planes = vtkPlaneCollection()
planes.AddItem(plane)

# vtkPolyDataPlaneClipper
clipper = vtkPolyDataPlaneClipper()
clipper.SetInputConnection(sphere.GetOutputPort())
clipper.SetPlane(plane)
clipper.SetBatchSize(10000)
clipper.CappingOn()

# Compare to vtkClipClosedSurface
closedClipper = vtkClipClosedSurface()
closedClipper.SetInputConnection(sphere.GetOutputPort())
closedClipper.SetClippingPlanes(planes)

# Compare to vtkClipPolyData
oldClipper = vtkClipPolyData()
oldClipper.SetInputConnection(sphere.GetOutputPort())
oldClipper.SetValue(0.0)
oldClipper.SetClipFunction(plane)

# Time execution
timer = vtkTimerLog()
timer.StartTimer()
clipper.Update()
timer.StopTimer()
print("vtkPolyDataPlaneClipper Execution time: ", timer.GetElapsedTime())

#timer.StartTimer()
#closedClipper.Update()
#timer.StopTimer()
#print("vtkClipClosedSurface Execution time: ", timer.GetElapsedTime())

#timer.StartTimer()
#oldClipper.Update()
#timer.StopTimer()
#print("vtkClipPolyData Execution time: ", timer.GetElapsedTime())

# Display the clipped cells
mapper = vtkPolyDataMapper()
mapper.SetInputConnection(clipper.GetOutputPort())

actor = vtkActor()
actor.SetMapper(mapper)

# Display the cap
capMapper = vtkPolyDataMapper()
capMapper.SetInputConnection(clipper.GetOutputPort(1))

capActor = vtkActor()
capActor.SetMapper(capMapper)
capActor.GetProperty().SetColor(1,0,0)

# Add the actors to the renderer, set the background and size
ren0.AddActor(actor)
ren0.AddActor(capActor)

ren0.SetBackground(0,0,0)
renWin.SetSize(300,300)
ren0.ResetCamera()
iren.Initialize()

renWin.Render()
iren.Start()
# --- end of script --
