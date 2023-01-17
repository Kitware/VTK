#!/usr/bin/env python
from vtkmodules.vtkCommonCore import vtkPoints
from vtkmodules.vtkCommonDataModel import (
    vtkCellArray,
    vtkPolyData,
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

# Create the RenderWindow, Renderer and both Actors
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
points = vtkPoints()
points.InsertPoint(0,0.0,0.0,0.0)
verts = vtkCellArray()
verts.InsertNextCell(1)
verts.InsertCellPoint(0)
polyData = vtkPolyData()
polyData.SetPoints(points)
polyData.SetVerts(verts)
mapper = vtkPolyDataMapper()
mapper.SetInputData(polyData)
actor = vtkActor()
actor.SetMapper(mapper)
actor.GetProperty().SetPointSize(8)
ren1.AddViewProp(actor)
renWin.Render()
# prevent the tk window from showing up then start the event loop
# --- end of script --
