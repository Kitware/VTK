#!/usr/bin/env python
from vtkmodules.vtkFiltersCore import (
    vtkGlyph3D,
    vtkProbeFilter,
    vtkStructuredGridOutlineFilter,
)
from vtkmodules.vtkFiltersGeneral import vtkCursor3D
from vtkmodules.vtkFiltersSources import vtkConeSource
from vtkmodules.vtkIOParallel import vtkMultiBlockPLOT3DReader
from vtkmodules.vtkRenderingCore import (
    vtkActor,
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

ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# read data
reader = vtkMultiBlockPLOT3DReader()
reader.SetXYZFileName(VTK_DATA_ROOT + "/Data/combxyz.bin")
reader.SetQFileName(VTK_DATA_ROOT + "/Data/combq.bin")
reader.SetScalarFunctionNumber(110)
reader.Update()
output = reader.GetOutput().GetBlock(0)
# create outline
outlineF = vtkStructuredGridOutlineFilter()
outlineF.SetInputData(output)
outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInputConnection(outlineF.GetOutputPort())
outline = vtkActor()
outline.SetMapper(outlineMapper)
outline.GetProperty().SetColor(0,0,0)
# create cursor
cursor = vtkCursor3D()
cursor.SetModelBounds(output.GetBounds())
cursor.SetFocalPoint(output.GetCenter())
cursor.AllOff()
cursor.AxesOn()
cursor.OutlineOn()
cursor.XShadowsOn()
cursor.YShadowsOn()
cursor.ZShadowsOn()
cursorMapper = vtkPolyDataMapper()
cursorMapper.SetInputConnection(cursor.GetOutputPort())
cursorActor = vtkActor()
cursorActor.SetMapper(cursorMapper)
cursorActor.GetProperty().SetColor(1,0,0)
# create probe
probe = vtkProbeFilter()
probe.SetInputData(cursor.GetFocus())
probe.SetSourceData(output)
# create a cone geometry for glyph
cone = vtkConeSource()
cone.SetResolution(16)
cone.SetRadius(0.25)
# create glyph
glyph = vtkGlyph3D()
glyph.SetInputConnection(probe.GetOutputPort())
glyph.SetSourceConnection(cone.GetOutputPort())
glyph.SetVectorModeToUseVector()
glyph.SetScaleModeToScaleByScalar()
glyph.SetScaleFactor(.0002)
glyphMapper = vtkPolyDataMapper()
glyphMapper.SetInputConnection(glyph.GetOutputPort())
glyphActor = vtkActor()
glyphActor.SetMapper(glyphMapper)
ren1.AddActor(outline)
ren1.AddActor(cursorActor)
ren1.AddActor(glyphActor)
ren1.SetBackground(1.0,1.0,1.0)
renWin.SetSize(200,200)
ren1.ResetCamera()
ren1.GetActiveCamera().Elevation(60)
ren1.ResetCameraClippingRange()
renWin.Render()
iren.Initialize()
# prevent the tk window from showing up then start the event loop
# --- end of script --
