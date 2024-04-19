#!/usr/bin/env python
from vtkmodules.vtkCommonCore import (
    vtkFloatArray,
    vtkPoints,
)
from vtkmodules.vtkCommonDataModel import vtkPolyData
from vtkmodules.vtkFiltersCore import vtkExecutionTimer
from vtkmodules.vtkImagingHybrid import vtkShepardMethod
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkDataSetMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtkRenderer()
ren1.SetViewport(0,0,.3333,1)
ren2 = vtkRenderer()
ren2.SetViewport(0.3333,0,.6667,1)
ren3 = vtkRenderer()
ren3.SetViewport(.6667,0,1,1)
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
renWin.AddRenderer(ren2)
renWin.AddRenderer(ren3)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Create some points and scalars. Make sure that some
# of the points lie precisely on the volume points. This
# tests special interpolation cases.
#
points = vtkPoints()
points.InsertPoint(0, -1, -1, -1)
points.InsertPoint(1,  1, -1, -1)
points.InsertPoint(2,  0,  0, -1)
points.InsertPoint(3, -1,  1, -1)
points.InsertPoint(4,  1,  1, -1)
points.InsertPoint(5, -1, -1,  0)
points.InsertPoint(6,  1, -1,  0)
points.InsertPoint(7,  0,  0,  0)
points.InsertPoint(8, -1,  1,  0)
points.InsertPoint(9,  1,  1,  0)
points.InsertPoint(10, -1, -1,  1)
points.InsertPoint(11,  1, -1,  1)
points.InsertPoint(12,  0,  0,  1)
points.InsertPoint(13, -1,  1,  1)
points.InsertPoint(14,  1,  1,  1)

scalars = vtkFloatArray()
scalars.InsertValue(0, 5)
scalars.InsertValue(1, 5)
scalars.InsertValue(2, 10)
scalars.InsertValue(3, 5)
scalars.InsertValue(4, 5)
scalars.InsertValue(5, 10)
scalars.InsertValue(6, 10)
scalars.InsertValue(7, 20)
scalars.InsertValue(8, 10)
scalars.InsertValue(9, 10)
scalars.InsertValue(10, 20)
scalars.InsertValue(11, 20)
scalars.InsertValue(12, 40)
scalars.InsertValue(13, 20)
scalars.InsertValue(14, 20)

profile = vtkPolyData()
profile.SetPoints(points)
profile.GetPointData().SetScalars(scalars)

# Interpolate the points across the volume.
#
dim = 501
dim = 51

shepard1 = vtkShepardMethod()
shepard1.SetInputData(profile)
shepard1.SetModelBounds(-2,2, -2,2, -1,1)
shepard1.SetSampleDimensions(dim,dim,dim)
shepard1.SetNullValue(0)
shepard1.SetMaximumDistance(1)
shepard1.SetPowerParameter(1)

timer = vtkExecutionTimer()
timer.SetFilter(shepard1)
shepard1.Update()
wallClock = timer.GetElapsedWallClockTime()
print ("Shephard (P=1):", wallClock)

mapper1 = vtkDataSetMapper()
mapper1.SetInputConnection(shepard1.GetOutputPort())
mapper1.SetScalarRange(0,40)

block1 = vtkActor()
block1.SetMapper(mapper1)

shepard2 = vtkShepardMethod()
shepard2.SetInputData(profile)
shepard2.SetModelBounds(-2,2, -2,2, -1,1)
shepard2.SetSampleDimensions(dim,dim,dim)
shepard2.SetNullValue(0)
shepard2.SetMaximumDistance(1)

timer = vtkExecutionTimer()
timer.SetFilter(shepard2)
shepard2.Update()
wallClock = timer.GetElapsedWallClockTime()
print ("Shephard (P=2):", wallClock)

mapper2 = vtkDataSetMapper()
mapper2.SetInputConnection(shepard2.GetOutputPort())
mapper2.SetScalarRange(0,40)

block2 = vtkActor()
block2.SetMapper(mapper2)

shepard3 = vtkShepardMethod()
shepard3.SetInputData(profile)
shepard3.SetModelBounds(-2,2, -2,2, -1,1)
shepard3.SetSampleDimensions(dim,dim,dim)
shepard3.SetNullValue(0)
shepard3.SetMaximumDistance(1)
shepard3.SetPowerParameter(3)

timer = vtkExecutionTimer()
timer.SetFilter(shepard3)
shepard3.Update()
wallClock = timer.GetElapsedWallClockTime()
print ("Shephard (P=3):", wallClock)

mapper3 = vtkDataSetMapper()
mapper3.SetInputConnection(shepard3.GetOutputPort())
mapper3.SetScalarRange(0,40)

block3 = vtkActor()
block3.SetMapper(mapper3)

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(block1)
ren1.SetBackground(1,1,1)
ren2.AddActor(block2)
ren2.SetBackground(1,1,1)
ren3.AddActor(block3)
ren3.SetBackground(1,1,1)

renWin.SetSize(900,300)
cam = ren1.GetActiveCamera()
cam.SetPosition(1,1,1)
ren1.ResetCamera()
ren2.SetActiveCamera(cam)
ren3.SetActiveCamera(cam)
renWin.Render()

# render the image
#
renWin.Render()

# --- end of script --
#iren.Start()
