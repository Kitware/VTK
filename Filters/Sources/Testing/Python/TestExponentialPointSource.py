#!/usr/bin/env python
import vtk

# Control the size of the test. Bigger numbers create more points.
numPts = 1000
radius = 0.5

# Create nine point sources with exponential distributions.
# Each has a different rate (lambda) factor. The nine
# sources are regularly placed within a cube, and then
# appended together.
ps0 = vtk.vtkPointSource()
ps0.SetDistributionToExponential()
ps0.SetCenter(-1,-1,-1)
ps0.SetRadius(radius)
ps0.SetNumberOfPoints(numPts)
ps0.SetLambda(.1)

ps1 = vtk.vtkPointSource()
ps1.SetDistributionToExponential()
ps1.SetCenter(1,-1,-1)
ps1.SetRadius(radius)
ps1.SetNumberOfPoints(numPts)
ps1.SetLambda(-10)

ps2 = vtk.vtkPointSource()
ps2.SetDistributionToExponential()
ps2.SetCenter(1,1,-1)
ps2.SetRadius(radius)
ps2.SetNumberOfPoints(numPts)
ps2.SetLambda(0.6)

ps3 = vtk.vtkPointSource()
ps3.SetDistributionToExponential()
ps3.SetCenter(-1,1,-1)
ps3.SetRadius(radius)
ps3.SetNumberOfPoints(numPts)
ps3.SetLambda(0.8)

ps4 = vtk.vtkPointSource()
ps4.SetDistributionToExponential()
ps4.SetCenter(0,0,0)
ps4.SetRadius(radius)
ps4.SetNumberOfPoints(numPts)
ps4.SetLambda(1.0)

ps5 = vtk.vtkPointSource()
ps5.SetDistributionToExponential()
ps5.SetCenter(-1,-1,1)
ps5.SetRadius(radius)
ps5.SetNumberOfPoints(numPts)
ps5.SetLambda(2)

ps6 = vtk.vtkPointSource()
ps6.SetDistributionToExponential()
ps6.SetCenter(1,-1,1)
ps6.SetRadius(radius)
ps6.SetNumberOfPoints(numPts)
ps6.SetLambda(4)

ps7 = vtk.vtkPointSource()
ps7.SetDistributionToExponential()
ps7.SetCenter(1,1,1)
ps7.SetRadius(radius)
ps7.SetNumberOfPoints(numPts)
ps7.SetLambda(6)

ps8 = vtk.vtkPointSource()
ps8.SetDistributionToExponential()
ps8.SetCenter(-1,1,1)
ps8.SetRadius(radius)
ps8.SetNumberOfPoints(numPts)
ps8.SetLambda(10)

# Comment out two point sources which are aligned along
# the camera view vector and the center point cloud -
# they just clutter the display.
appendF = vtk.vtkAppendPolyData()
#appendF.AddInputConnection(ps0.GetOutputPort())
appendF.AddInputConnection(ps1.GetOutputPort())
appendF.AddInputConnection(ps2.GetOutputPort())
appendF.AddInputConnection(ps3.GetOutputPort())
appendF.AddInputConnection(ps4.GetOutputPort())
appendF.AddInputConnection(ps5.GetOutputPort())
appendF.AddInputConnection(ps6.GetOutputPort())
#appendF.AddInputConnection(ps7.GetOutputPort())
appendF.AddInputConnection(ps8.GetOutputPort())

mapper = vtk.vtkPolyDataMapper()
mapper.SetInputConnection(appendF.GetOutputPort())

actor = vtk.vtkActor()
actor.SetMapper(mapper)

# Create the RenderWindow, Renderer and Interactive Renderer
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

ren1.AddActor(actor)
ren1.SetBackground(0, 0, 0)

renWin.SetSize(300, 300)

renWin.Render()
ren1.GetActiveCamera().SetPosition(1,1,1)
ren1.ResetCamera()
renWin.Render()

iren.Initialize()
iren.Start()
