#!/usr/bin/env python
from vtkmodules.vtkCommonCore import (
    vtkFloatArray,
    vtkMath,
)
from vtkmodules.vtkFiltersProgrammable import vtkProgrammableAttributeDataFilter
from vtkmodules.vtkFiltersSources import vtkSphereSource
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
from vtkmodules.vtkRenderingAnnotation import vtkScalarBarActor
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# create pipeline
#
# create sphere to color
sphere = vtkSphereSource()
sphere.SetThetaResolution(20)
sphere.SetPhiResolution(40)
def colorCells(obj=None, event=""):
    randomColorGenerator = vtkMath()
    input = randomColors.GetInput()
    output = randomColors.GetOutput()
    numCells = input.GetNumberOfCells()
    colors = vtkFloatArray()
    colors.SetNumberOfTuples(numCells)
    i = 0
    while i < numCells:
        colors.SetValue(i,randomColorGenerator.Random(0,1))
        i = i + 1

    output.GetCellData().CopyScalarsOff()
    output.GetCellData().PassData(input.GetCellData())
    output.GetCellData().SetScalars(colors)

# Compute random scalars (colors) for each cell
randomColors = vtkProgrammableAttributeDataFilter()
randomColors.SetInputConnection(sphere.GetOutputPort())
randomColors.SetExecuteMethod(colorCells)
# mapper and actor
mapper = vtkPolyDataMapper()
mapper.SetInputConnection(randomColors.GetOutputPort())
mapper.SetScalarRange(randomColors.GetPolyDataOutput().GetScalarRange())
sphereActor = vtkActor()
sphereActor.SetMapper(mapper)
# Create a scalar bar
scalarBar = vtkScalarBarActor()
scalarBar.SetLookupTable(mapper.GetLookupTable())
scalarBar.SetTitle("Temperature")
scalarBar.GetPositionCoordinate().SetCoordinateSystemToNormalizedViewport()
scalarBar.GetPositionCoordinate().SetValue(0.1,0.01)
scalarBar.SetOrientationToHorizontal()
scalarBar.SetWidth(0.8)
scalarBar.SetHeight(0.17)
# Test the Get/Set Position
scalarBar.SetPosition(scalarBar.GetPosition())
# Create graphics stuff
# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
ren1.AddActor(sphereActor)
ren1.AddViewProp(scalarBar)
renWin.SetSize(350,350)
# render the image
#
ren1.ResetCamera()
ren1.GetActiveCamera().Zoom(1.5)
renWin.Render()
scalarBar.SetNumberOfLabels(8)
renWin.Render()
# --- end of script --
