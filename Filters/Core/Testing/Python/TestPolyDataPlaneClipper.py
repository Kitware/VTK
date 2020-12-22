#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Control test size
res = 1024

# Create the RenderWindow, Renderers and both Actors
ren0 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren0)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Create a synthetic sphere
sphere = vtk.vtkSphereSource()
sphere.SetCenter(0.0, 0.0, 0.0)
sphere.SetRadius(0.25)
sphere.SetThetaResolution(2*res)
sphere.SetPhiResolution(res)
sphere.Update()
print("Processing: ", sphere.GetOutput().GetNumberOfCells(), " triangles")

# The cut plane
plane = vtk.vtkPlane()
plane.SetOrigin(0, 0, 0)
plane.SetNormal(-1, -1, -1)
planes = vtk.vtkPlaneCollection()
planes.AddItem(plane)

# vtkPolyDataPlaneClipper
clipper = vtk.vtkPolyDataPlaneClipper()
clipper.SetInputConnection(sphere.GetOutputPort())
clipper.SetPlane(plane)
clipper.SetBatchSize(10000)
clipper.CappingOn()

# Compare to vtkClipClosedSurface
closedClipper = vtk.vtkClipClosedSurface()
closedClipper.SetInputConnection(sphere.GetOutputPort())
closedClipper.SetClippingPlanes(planes)

# Compare to vtkClipPolyData
oldClipper = vtk.vtkClipPolyData()
oldClipper.SetInputConnection(sphere.GetOutputPort())
oldClipper.SetValue(0.0)
oldClipper.SetClipFunction(plane)

# Time execution
timer = vtk.vtkTimerLog()
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
mapper = vtk.vtkPolyDataMapper()
mapper.SetInputConnection(clipper.GetOutputPort())

actor = vtk.vtkActor()
actor.SetMapper(mapper)

# Display the cap
capMapper = vtk.vtkPolyDataMapper()
capMapper.SetInputConnection(clipper.GetOutputPort(1))

capActor = vtk.vtkActor()
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
