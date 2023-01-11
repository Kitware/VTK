#!/usr/bin/env python
from vtkmodules.vtkCommonColor import vtkNamedColors
from vtkmodules.vtkFiltersCore import (
    vtkPolyDataNormals,
    vtkWindowedSincPolyDataFilter,
)
from vtkmodules.vtkFiltersGeneral import (
    vtkBrownianPoints,
    vtkWarpVector,
)
from vtkmodules.vtkFiltersModeling import vtkRotationalExtrusionFilter
from vtkmodules.vtkFiltersSources import vtkLineSource
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkCamera,
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

def GetRGBColor(colorName):
    '''
        Return the red, green and blue components for a
        color as doubles.
    '''
    rgb = [0.0, 0.0, 0.0]  # black
    vtkNamedColors().GetColorRGB(colorName, rgb)
    return rgb

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# create a semi-cylinder
#
line = vtkLineSource()
line.SetPoint1(0, 1, 0)
line.SetPoint2(0, 1, 2)
line.SetResolution(10)

lineSweeper = vtkRotationalExtrusionFilter()
lineSweeper.SetResolution(20)
lineSweeper.SetInputConnection(line.GetOutputPort())
lineSweeper.SetAngle(270)

bump = vtkBrownianPoints()
bump.SetInputConnection(lineSweeper.GetOutputPort())

warp = vtkWarpVector()
warp.SetInputConnection(bump.GetOutputPort())
warp.SetScaleFactor(.2)

smooth = vtkWindowedSincPolyDataFilter()
smooth.SetInputConnection(warp.GetOutputPort())
smooth.SetNumberOfIterations(20)
smooth.BoundarySmoothingOn()
smooth.SetFeatureAngle(120)
smooth.SetEdgeAngle(90)
smooth.SetPassBand(0.1)

normals = vtkPolyDataNormals()
normals.SetInputConnection(smooth.GetOutputPort())

cylMapper = vtkPolyDataMapper()
cylMapper.SetInputConnection(normals.GetOutputPort())

cylActor = vtkActor()
cylActor.SetMapper(cylMapper)
cylActor.GetProperty().SetInterpolationToGouraud()
cylActor.GetProperty().SetInterpolationToFlat()
cylActor.GetProperty().SetColor(GetRGBColor('beige'))

originalMapper = vtkPolyDataMapper()
originalMapper.SetInputConnection(bump.GetOutputPort())
originalActor = vtkActor()
originalActor.SetMapper(originalMapper)
originalActor.GetProperty().SetInterpolationToFlat()

cylActor.GetProperty().SetColor(GetRGBColor('tomato'))

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(cylActor)
# ren1 AddActor originalActor
ren1.SetBackground(1, 1, 1)

renWin.SetSize(200, 300)

camera = vtkCamera()
camera.SetClippingRange(0.576398, 28.8199)
camera.SetFocalPoint(0.0463079, -0.0356571, 1.01993)
camera.SetPosition(-2.47044, 2.39516, -3.56066)
camera.SetViewUp(0.607296, -0.513537, -0.606195)

ren1.SetActiveCamera(camera)

# render the image
#
iren.Initialize()
iren.Start()
