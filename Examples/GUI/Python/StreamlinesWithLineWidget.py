#!/usr/bin/env python

# This example demonstrates how to use the vtkLineWidget to seed and
# manipulate streamlines. Two line widgets are created. One is invoked
# by pressing 'i', the other by pressing 'L' (capital). Both can exist
# together.

import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Start by loading some data.
pl3d = vtk.vtkPLOT3DReader()
pl3d.SetXYZFileName(VTK_DATA_ROOT + "/Data/combxyz.bin")
pl3d.SetQFileName(VTK_DATA_ROOT + "/Data/combq.bin")
pl3d.SetScalarFunctionNumber(100)
pl3d.SetVectorFunctionNumber(202)
pl3d.Update()

# The line widget is used seed the streamlines.
lineWidget = vtk.vtkLineWidget()
seeds = vtk.vtkPolyData()
lineWidget.SetInput(pl3d.GetOutput())
lineWidget.SetAlignToYAxis()
lineWidget.PlaceWidget()
lineWidget.GetPolyData(seeds)
lineWidget.ClampToBoundsOn()

rk4 = vtk.vtkRungeKutta4()
streamer = vtk.vtkStreamLine()
streamer.SetInput(pl3d.GetOutput())
streamer.SetSource(seeds)
streamer.SetMaximumPropagationTime(100)
streamer.SetIntegrationStepLength(.2)
streamer.SetStepLength(.001)
streamer.SetNumberOfThreads(1)
streamer.SetIntegrationDirectionToForward()
streamer.VorticityOn()
streamer.SetIntegrator(rk4)
rf = vtk.vtkRibbonFilter()
rf.SetInput(streamer.GetOutput())
rf.SetWidth(0.1)
rf.SetWidthFactor(5)
streamMapper = vtk.vtkPolyDataMapper()
streamMapper.SetInput(rf.GetOutput())
streamMapper.SetScalarRange(pl3d.GetOutput().GetScalarRange())
streamline = vtk.vtkActor()
streamline.SetMapper(streamMapper)
streamline.VisibilityOff()

# The second line widget is used seed more streamlines.
lineWidget2 = vtk.vtkLineWidget()
seeds2 = vtk.vtkPolyData()
lineWidget2.SetInput(pl3d.GetOutput())
lineWidget2.PlaceWidget()
lineWidget2.GetPolyData(seeds2)
lineWidget2.SetKeyPressActivationValue('L')

streamer2 = vtk.vtkStreamLine()
streamer2.SetInput(pl3d.GetOutput())
streamer2.SetSource(seeds2)
streamer2.SetMaximumPropagationTime(100)
streamer2.SetIntegrationStepLength(.2)
streamer2.SetStepLength(.001)
streamer2.SetNumberOfThreads(1)
streamer2.SetIntegrationDirectionToForward()
streamer2.VorticityOn()
streamer2.SetIntegrator(rk4)
rf2 = vtk.vtkRibbonFilter()
rf2.SetInput(streamer2.GetOutput())
rf2.SetWidth(0.1)
rf2.SetWidthFactor(5)
streamMapper2 = vtk.vtkPolyDataMapper()
streamMapper2.SetInput(rf2.GetOutput())
streamMapper2.SetScalarRange(pl3d.GetOutput().GetScalarRange())
streamline2 = vtk.vtkActor()
streamline2.SetMapper(streamMapper2)
streamline2.VisibilityOff()

outline = vtk.vtkStructuredGridOutlineFilter()
outline.SetInput(pl3d.GetOutput())
outlineMapper = vtk.vtkPolyDataMapper()
outlineMapper.SetInput(outline.GetOutput())
outlineActor = vtk.vtkActor()
outlineActor.SetMapper(outlineMapper)

# Create the RenderWindow, Renderer and both Actors
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Callback functions that actually generate streamlines.
def BeginInteraction(obj, event):
    global streamline
    streamline.VisibilityOn() 

def GenerateStreamlines(obj, event):
    global seeds, renWin
    obj.GetPolyData(seeds)
    renWin.Render()

def BeginInteraction2(obj, event):
    global streamline2
    streamline2.VisibilityOn()

def GenerateStreamlines2(obj, event):
    global seeds2, renWin
    obj.GetPolyData(seeds2)
    renWin.Render()

# Associate the line widget with the interactor and setup callbacks.
lineWidget.SetInteractor(iren)
lineWidget.AddObserver("StartInteractionEvent", BeginInteraction)
lineWidget.AddObserver("InteractionEvent", GenerateStreamlines)

lineWidget2.SetInteractor(iren)
lineWidget2.AddObserver("StartInteractionEvent", BeginInteraction2)
lineWidget2.AddObserver("EndInteractionEvent", GenerateStreamlines2)

# Add the actors to the renderer, set the background and size
ren.AddActor(outlineActor)
ren.AddActor(streamline)
ren.AddActor(streamline2)

ren.SetBackground(1, 1, 1)
renWin.SetSize(300, 300)
ren.SetBackground(0.1, 0.2, 0.4)

cam1 = ren.GetActiveCamera()
cam1.SetClippingRange(3.95297, 50)
cam1.SetFocalPoint(9.71821, 0.458166, 29.3999)
cam1.SetPosition(2.7439, -37.3196, 38.7167)
cam1.SetViewUp(-0.16123, 0.264271, 0.950876)

iren.Initialize()
renWin.Render()
iren.Start()
