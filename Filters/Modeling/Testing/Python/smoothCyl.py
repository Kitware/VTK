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
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# create a semi-cylinder
#
line = vtk.vtkLineSource()
line.SetPoint1(0, 1, 0)
line.SetPoint2(0, 1, 2)
line.SetResolution(10)

lineSweeper = vtk.vtkRotationalExtrusionFilter()
lineSweeper.SetResolution(20)
lineSweeper.SetInputConnection(line.GetOutputPort())
lineSweeper.SetAngle(270)

bump = vtk.vtkBrownianPoints()
bump.SetInputConnection(lineSweeper.GetOutputPort())

warp = vtk.vtkWarpVector()
warp.SetInputConnection(bump.GetOutputPort())
warp.SetScaleFactor(.2)

smooth = vtk.vtkSmoothPolyDataFilter()
smooth.SetInputConnection(warp.GetOutputPort())
smooth.SetNumberOfIterations(50)
smooth.BoundarySmoothingOn()
smooth.SetFeatureAngle(120)
smooth.SetEdgeAngle(90)
smooth.SetRelaxationFactor(.025)

normals = vtk.vtkPolyDataNormals()
normals.SetInputConnection(smooth.GetOutputPort())

cylMapper = vtk.vtkPolyDataMapper()
cylMapper.SetInputConnection(normals.GetOutputPort())

cylActor = vtk.vtkActor()
cylActor.SetMapper(cylMapper)
cylActor.GetProperty().SetInterpolationToGouraud()
cylActor.GetProperty().SetInterpolationToFlat()
cylActor.GetProperty().SetColor(GetRGBColor('beige'))

originalMapper = vtk.vtkPolyDataMapper()
originalMapper.SetInputConnection(bump.GetOutputPort())

originalActor = vtk.vtkActor()
originalActor.SetMapper(originalMapper)
originalActor.GetProperty().SetInterpolationToFlat()

cylActor.GetProperty().SetColor(GetRGBColor('tomato'))

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(cylActor)
# ren1.AddActor(originalActor)
ren1.SetBackground(1, 1, 1)

renWin.SetSize(200, 300)

camera = vtk.vtkCamera()
camera.SetClippingRange(0.576398, 28.8199)
camera.SetFocalPoint(0.0463079, -0.0356571, 1.01993)
camera.SetPosition(-2.47044, 2.39516, -3.56066)
camera.SetViewUp(0.607296, -0.513537, -0.606195)

ren1.SetActiveCamera(camera)

# render the image
#
iren.Initialize()
#iren.Start()
