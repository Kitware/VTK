#!/usr/bin/env python
from vtkmodules.vtkCommonCore import vtkPoints
from vtkmodules.vtkCommonDataModel import (
    vtkCellArray,
    vtkPlane,
    vtkPolyData,
)
from vtkmodules.vtkFiltersCore import (
    vtkAppendPolyData,
    vtkCleanPolyData,
    vtkCutter,
    vtkStripper,
)
from vtkmodules.vtkFiltersModeling import vtkRuledSurfaceFilter
from vtkmodules.vtkFiltersSources import vtkSphereSource
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

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
sphere = vtkSphereSource()
sphere.SetPhiResolution(15)
sphere.SetThetaResolution(30)
plane = vtkPlane()
plane.SetNormal(1,0,0)
cut = vtkCutter()
cut.SetInputConnection(sphere.GetOutputPort())
cut.SetCutFunction(plane)
cut.GenerateCutScalarsOn()
strip = vtkStripper()
strip.SetInputConnection(cut.GetOutputPort())
points = vtkPoints()
points.InsertPoint(0,1,0,0)
lines = vtkCellArray()
lines.InsertNextCell(2)
#number of points
lines.InsertCellPoint(0)
lines.InsertCellPoint(0)
tip = vtkPolyData()
tip.SetPoints(points)
tip.SetLines(lines)
appendPD = vtkAppendPolyData()
appendPD.AddInputConnection(strip.GetOutputPort())
appendPD.AddInputData(tip)
# extrude profile to make coverage
#
extrude = vtkRuledSurfaceFilter()
extrude.SetInputConnection(appendPD.GetOutputPort())
extrude.SetRuledModeToPointWalk()
clean = vtkCleanPolyData()
clean.SetInputConnection(extrude.GetOutputPort())
clean.ConvertPolysToLinesOff()
mapper = vtkPolyDataMapper()
mapper.SetInputConnection(clean.GetOutputPort())
mapper.ScalarVisibilityOff()
actor = vtkActor()
actor.SetMapper(mapper)
actor.GetProperty().SetOpacity(.4)
ren1.AddActor(actor)
renWin.SetSize(200,200)
renWin.Render()
# render the image
#
iren.Initialize()
# prevent the tk window from showing up then start the event loop
# --- end of script --
