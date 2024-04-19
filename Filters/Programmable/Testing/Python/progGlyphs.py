#!/usr/bin/env python
from vtkmodules.vtkFiltersCore import vtkElevationFilter
from vtkmodules.vtkFiltersProgrammable import vtkProgrammableGlyphFilter
from vtkmodules.vtkFiltersSources import (
    vtkPlaneSource,
    vtkSuperquadricSource,
)
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

res = 6
plane = vtkPlaneSource()
plane.SetResolution(res,res)
colors = vtkElevationFilter()
colors.SetInputConnection(plane.GetOutputPort())
colors.SetLowPoint(-0.25,-0.25,-0.25)
colors.SetHighPoint(0.25,0.25,0.25)
planeMapper = vtkPolyDataMapper()
planeMapper.SetInputConnection(colors.GetOutputPort())
planeActor = vtkActor()
planeActor.SetMapper(planeMapper)
planeActor.GetProperty().SetRepresentationToWireframe()
# procedure for generating glyphs
def Glyph():
    ptId = glypher.GetPointId()
    pd = glypher.GetPointData()
    x,y,z = glypher.GetPoint()
    length = glypher.GetInput(0).GetLength()
    scale = length/(2.0*res)
    squad.SetScale(scale,scale,scale)
    squad.SetCenter(x,y,z)
    squad.SetPhiRoundness(abs(x)*5.0)
    squad.SetThetaRoundness(abs(y)*5.0)
    squad.Update()

# create simple poly data so we can apply glyph
squad = vtkSuperquadricSource()
squad.Update()
glypher = vtkProgrammableGlyphFilter()
glypher.SetInputConnection(colors.GetOutputPort())
glypher.SetSourceData(squad.GetOutput())
glypher.SetGlyphMethod(Glyph)
glyphMapper = vtkPolyDataMapper()
glyphMapper.SetInputConnection(glypher.GetOutputPort())
glyphActor = vtkActor()
glyphActor.SetMapper(glyphMapper)
# Create the rendering stuff
#
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
ren1.AddActor(planeActor)
ren1.AddActor(glyphActor)
ren1.SetBackground(1,1,1)
renWin.SetSize(450,450)
renWin.Render()
ren1.GetActiveCamera().Zoom(1.5)
# Get handles to some useful objects
#
renWin.Render()
# prevent the tk window from showing up then start the event loop
# --- end of script --
