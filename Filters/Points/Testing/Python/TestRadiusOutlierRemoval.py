#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Interpolate onto a volume

# Parameters for debugging
NPts = 10000
math = vtk.vtkMath()
math.RandomSeed(31415)

# create pipeline
#
points = vtk.vtkBoundedPointSource()
points.SetNumberOfPoints(NPts)
points.ProduceRandomScalarsOn()
points.ProduceCellOutputOff()
points.Update()

# Reuse the locator
locator = vtk.vtkStaticPointLocator()
locator.SetDataSet(points.GetOutput())
locator.BuildLocator()

# Remove isolated points
removal = vtk.vtkRadiusOutlierRemoval()
removal.SetInputConnection(points.GetOutputPort())
removal.SetLocator(locator)
removal.SetRadius(0.1)
removal.SetNumberOfNeighbors(2)
removal.GenerateOutliersOn()

# Time execution
timer = vtk.vtkTimerLog()
timer.StartTimer()
removal.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Time to remove points: {0}".format(time))
print("   Number removed: {0}".format(removal.GetNumberOfPointsRemoved()))
print("   Original number of points: {0}".format(NPts))

# First output are the non-outliers
remMapper = vtk.vtkPointGaussianMapper()
remMapper.SetInputConnection(removal.GetOutputPort())
remMapper.EmissiveOff()
remMapper.SetScaleFactor(0.0)

remActor = vtk.vtkActor()
remActor.SetMapper(remMapper)

# Create an outline
outline = vtk.vtkOutlineFilter()
outline.SetInputConnection(points.GetOutputPort())

outlineMapper = vtk.vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = vtk.vtkActor()
outlineActor.SetMapper(outlineMapper)

# Second output are the outliers
remMapper1 = vtk.vtkPointGaussianMapper()
remMapper1.SetInputConnection(removal.GetOutputPort(1))
remMapper1.EmissiveOff()
remMapper1.SetScaleFactor(0.0)

remActor1 = vtk.vtkActor()
remActor1.SetMapper(remMapper1)

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
ren0.AddActor(remActor)
ren0.AddActor(outlineActor)
ren0.SetBackground(0.1, 0.2, 0.4)

ren1.AddActor(remActor1)
ren1.AddActor(outlineActor1)
ren1.SetBackground(0.1, 0.2, 0.4)

renWin.SetSize(500,250)

cam = ren0.GetActiveCamera()
cam.SetFocalPoint(1,1,1)
cam.SetPosition(0,0,0)
ren0.ResetCamera()

ren1.SetActiveCamera(cam)

iren.Initialize()

# render the image
#
renWin.Render()

iren.Start()
