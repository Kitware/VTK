#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

def GetRGBColor(colorName):
    '''
        Return the red, green and blue components for a
        color as doubles.
    '''
    rgb = [0.0, 0.0, 0.0]  # black
    vtk.vtkNamedColors().GetColorRGB(colorName, rgb)
    return rgb

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# create pipeline
#
reader = vtk.vtkStructuredPointsReader()
reader.SetFileName(VTK_DATA_ROOT + "/Data/ironProt.vtk")
iso = vtk.vtkRecursiveDividingCubes()
iso.SetInputConnection(reader.GetOutputPort())
iso.SetValue(128)
iso.SetDistance(.5)
iso.SetIncrement(2)
isoMapper = vtk.vtkPolyDataMapper()
isoMapper.SetInputConnection(iso.GetOutputPort())
isoMapper.ScalarVisibilityOff()
isoMapper.ImmediateModeRenderingOn()
isoActor = vtk.vtkActor()
isoActor.SetMapper(isoMapper)
isoActor.GetProperty().SetColor(GetRGBColor('bisque'))

outline = vtk.vtkOutlineFilter()
outline.SetInputConnection(reader.GetOutputPort())
outlineMapper = vtk.vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())
outlineActor = vtk.vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineActor.GetProperty().SetColor(GetRGBColor('black'))

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(outlineActor)
ren1.AddActor(isoActor)

renWin.SetSize(250, 250)

ren1.SetBackground(0.1, 0.2, 0.4)

renWin.Render()

cam1 = vtk.vtkCamera()
cam1.SetClippingRange(19.1589, 957.946)
cam1.SetFocalPoint(33.7014, 26.706, 30.5867)
cam1.SetPosition(150.841, 89.374, -107.462)
cam1.SetViewUp(-0.190015, 0.944614, 0.267578)
cam1.Dolly(3)

ren1.SetActiveCamera(cam1)

iren.Initialize()
#iren.Start()
