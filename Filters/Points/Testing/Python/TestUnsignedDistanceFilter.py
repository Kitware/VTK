#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Interpolate onto a volume

# Parameters for debugging
NPts = 100 #Keep test small
math = vtk.vtkMath()
math.RandomSeed(31415)
res = 50

polyData = vtk.vtkPolyData()
pts = vtk.vtkPoints()
pts.SetDataTypeToFloat()
pts.SetNumberOfPoints(NPts)
for i in range(0,NPts):
    pts.SetPoint(i,math.Random(-1,1),math.Random(-1,1),math.Random(-1,1))
polyData.SetPoints(pts);

# Generate signed distance function and contour it
dist = vtk.vtkUnsignedDistance()
dist.SetInputData(polyData)
dist.SetRadius(0.25) #how far out to propagate distance calculation
dist.SetDimensions(res,res,res)
dist.CappingOn()
dist.AdjustBoundsOn()
dist.SetAdjustDistance(0.01)

# Extract the surface with modified flying edges
fe = vtk.vtkFlyingEdges3D()
fe.SetInputConnection(dist.GetOutputPort())
fe.SetValue(0, 0.075)
fe.ComputeNormalsOff()

# Time the execution
timer = vtk.vtkTimerLog()
timer.StartTimer()
fe.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Points processed: {0}".format(NPts))
print("   Time to generate and extract distance function: {0}".format(time))
print(dist)

feMapper = vtk.vtkPolyDataMapper()
feMapper.SetInputConnection(fe.GetOutputPort())

feActor = vtk.vtkActor()
feActor.SetMapper(feMapper)

# Create an outline
outline = vtk.vtkOutlineFilter()
outline.SetInputConnection(fe.GetOutputPort())

outlineMapper = vtk.vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = vtk.vtkActor()
outlineActor.SetMapper(outlineMapper)

# Create the RenderWindow, Renderer and both Actors
#
ren0 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren0)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren0.AddActor(feActor)
ren0.AddActor(outlineActor)
ren0.SetBackground(0.1, 0.2, 0.4)

renWin.SetSize(250,250)

cam = ren0.GetActiveCamera()
cam.SetFocalPoint(1,-1,-1)
cam.SetPosition(0,0,0)
ren0.ResetCamera()

iren.Initialize()

# render the image
#
renWin.Render()

iren.Start()
