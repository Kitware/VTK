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

renWin = vtkRenderWindow()
renWin.SetSize(300, 150)

# We create two renderings: one for the legacy Hamming window
# and one for the new Nuttall window.
#
# Incorrect smoothing leads to one color dominating the surface visible on each side
# (this is expected in the left renderer, showing Hamming window result).
#
# Correct smoothing leads to white and red surface patches
# visible in about the same amount on both sides of the surface
# (this is expected in the right renderer, showing Nuttal window result).

windowFunctions = [vtkWindowedSincPolyDataFilter.HAMMING, vtkWindowedSincPolyDataFilter.NUTTALL]

for rendererIndex, windowFunction in enumerate(windowFunctions):

    # create a semi-cylinder
    #
    line = vtkLineSource()
    line.SetPoint1(0, 1, 0)
    line.SetPoint2(0, 1, 2)
    line.SetResolution(50)

    lineSweeper = vtkRotationalExtrusionFilter()
    lineSweeper.SetResolution(100)
    lineSweeper.SetInputConnection(line.GetOutputPort())
    lineSweeper.SetAngle(270)

    # add some random displacement to the surface

    bump = vtkBrownianPoints()
    bump.SetInputConnection(lineSweeper.GetOutputPort())

    warp = vtkWarpVector()
    warp.SetInputConnection(bump.GetOutputPort())
    warp.SetScaleFactor(.005)

    # apply smoothing

    smooth = vtkWindowedSincPolyDataFilter()
    smooth.SetInputConnection(warp.GetOutputPort())
    smooth.SetNumberOfIterations(20)
    smooth.BoundarySmoothingOn()
    smooth.SetFeatureAngle(120)
    smooth.SetEdgeAngle(90)
    smooth.SetPassBand(0.1)
    smooth.SetWindowFunction(windowFunction)

    # render

    normals = vtkPolyDataNormals()
    normals.SetInputConnection(smooth.GetOutputPort())

    smoothingOutputMapper = vtkPolyDataMapper()
    smoothingOutputMapper.SetInputConnection(normals.GetOutputPort())
    smoothingOutputActor = vtkActor()
    smoothingOutputActor.SetMapper(smoothingOutputMapper)
    smoothingOutputActor.GetProperty().SetInterpolationToGouraud()
    smoothingOutputActor.GetProperty().SetInterpolationToFlat()
    smoothingOutputActor.GetProperty().SetColor(GetRGBColor('beige'))

    smoothingInputMapper = vtkPolyDataMapper()
    smoothingInputMapper.SetInputConnection(warp.GetOutputPort())
    smoothingInputActor = vtkActor()
    smoothingInputActor.SetMapper(smoothingInputMapper)
    smoothingInputActor.GetProperty().SetInterpolationToFlat()
    smoothingOutputActor.GetProperty().SetColor(GetRGBColor('tomato'))

    ren = vtkRenderer()
    ren.SetViewport(rendererIndex / len(windowFunctions),0,(rendererIndex + 1) / len(windowFunctions),1.0)
    ren.AddActor(smoothingOutputActor)
    ren.AddActor(smoothingInputActor)
    ren.SetBackground(1, 1, 1)

    camera = vtkCamera()
    camera.SetClippingRange(0.576398, 28.8199)
    camera.SetFocalPoint(0.0463079, -0.0356571, 1.01993)
    camera.SetPosition(-2.47044, 2.39516, -3.56066)
    camera.SetViewUp(0.607296, -0.513537, -0.606195)

    ren.SetActiveCamera(camera)
    renWin.AddRenderer(ren)

# render the image
#
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
iren.Initialize()
iren.Start()
