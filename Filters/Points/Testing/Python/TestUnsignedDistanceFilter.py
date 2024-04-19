#!/usr/bin/env python
from vtkmodules.vtkCommonCore import (
    vtkMath,
    vtkPoints,
)
from vtkmodules.vtkCommonDataModel import vtkPolyData
from vtkmodules.vtkCommonSystem import vtkTimerLog
from vtkmodules.vtkFiltersCore import vtkFlyingEdges3D
from vtkmodules.vtkFiltersModeling import vtkOutlineFilter
from vtkmodules.vtkFiltersPoints import vtkUnsignedDistance
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
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Interpolate onto a volume

# Parameters for debugging
NPts = 100 #Keep test small
math = vtkMath()
math.RandomSeed(31415)
res = 50

polyData = vtkPolyData()
pts = vtkPoints()
pts.SetDataTypeToFloat()
pts.SetNumberOfPoints(NPts)
for i in range(0,NPts):
    pts.SetPoint(i,math.Random(-1,1),math.Random(-1,1),math.Random(-1,1))
polyData.SetPoints(pts);

# Generate signed distance function and contour it
dist = vtkUnsignedDistance()
dist.SetInputData(polyData)
dist.SetRadius(0.25) #how far out to propagate distance calculation
dist.SetDimensions(res,res,res)
dist.CappingOn()
dist.AdjustBoundsOn()
dist.SetAdjustDistance(0.01)

# Extract the surface with modified flying edges
fe = vtkFlyingEdges3D()
fe.SetInputConnection(dist.GetOutputPort())
fe.SetValue(0, 0.075)
fe.ComputeNormalsOff()

# Time the execution
timer = vtkTimerLog()
timer.StartTimer()
fe.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Points processed: {0}".format(NPts))
print("   Time to generate and extract distance function: {0}".format(time))
print(dist)

feMapper = vtkPolyDataMapper()
feMapper.SetInputConnection(fe.GetOutputPort())

feActor = vtkActor()
feActor.SetMapper(feMapper)

# Create an outline
outline = vtkOutlineFilter()
outline.SetInputConnection(fe.GetOutputPort())

outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)

# Create the RenderWindow, Renderer and both Actors
#
ren0 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren0)
iren = vtkRenderWindowInteractor()
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
