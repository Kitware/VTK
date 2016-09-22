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
points.ProduceRandomScalarsOn()
points.ProduceCellOutputOff()
points.Update()

# Subsample
subsample = vtk.vtkVoxelGrid()
subsample.SetInputConnection(points.GetOutputPort())
subsample.SetConfigurationStyleToAutomatic()

# Time execution
timer = vtk.vtkTimerLog()
timer.StartTimer()
subsample.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Time to subsample: {0}".format(time))
print("   Original number of points: {0}".format(NPts))
print("   Final number of points: {0}".format(subsample.GetOutput().GetNumberOfPoints()))

# Output the original points
subMapper = vtk.vtkPointGaussianMapper()
subMapper.SetInputConnection(points.GetOutputPort())
subMapper.EmissiveOff()
subMapper.SetScaleFactor(0.0)

subActor = vtk.vtkActor()
subActor.SetMapper(subMapper)

# Create an outline
outline = vtk.vtkOutlineFilter()
outline.SetInputConnection(points.GetOutputPort())

outlineMapper = vtk.vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = vtk.vtkActor()
outlineActor.SetMapper(outlineMapper)

# Output the subsampled points
subMapper1 = vtk.vtkPointGaussianMapper()
subMapper1.SetInputConnection(subsample.GetOutputPort())
subMapper1.EmissiveOff()
subMapper1.SetScaleFactor(0.0)

subActor1 = vtk.vtkActor()
subActor1.SetMapper(subMapper1)

# Create an outline
outline1 = vtk.vtkOutlineFilter()
outline1.SetInputConnection(points.GetOutputPort())

outlineMapper1 = vtk.vtkPolyDataMapper()
outlineMapper1.SetInputConnection(outline1.GetOutputPort())

outlineActor1 = vtk.vtkActor()
outlineActor1.SetMapper(outlineMapper1)

# Create the RenderWindow, Renderer and both Actors
#
ren0 = vtk.vtkRenderer()
ren0.SetViewport(0,0,.5,1)
ren1 = vtk.vtkRenderer()
ren1.SetViewport(0.5,0,1,1)
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren0)
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren0.AddActor(subActor)
ren0.AddActor(outlineActor)
ren0.SetBackground(0.1, 0.2, 0.4)

ren1.AddActor(subActor1)
ren1.AddActor(outlineActor1)
ren1.SetBackground(0.1, 0.2, 0.4)

renWin.SetSize(500,250)

cam = ren0.GetActiveCamera()
cam.SetFocalPoint(0,0,0)
cam.SetPosition(1,1,1)
ren0.ResetCamera()

ren1.SetActiveCamera(cam)

iren.Initialize()

# render the image
#
renWin.Render()

#iren.Start()
