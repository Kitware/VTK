#!/usr/bin/env python
from vtkmodules.vtkFiltersGeneral import vtkRotationFilter
from vtkmodules.vtkFiltersSources import vtkConeSource
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkDataSetMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

cone = vtkConeSource()
cone.SetRadius(0.05)
cone.SetHeight(0.25)
cone.SetResolution(256)
cone.SetCenter(0.15,0.0,0.15)
rotate = vtkRotationFilter()
rotate.SetInputConnection(cone.GetOutputPort())
rotate.SetAxisToZ()
rotate.SetCenter(0.0,0.0,0.0)
rotate.SetAngle(45)
rotate.SetNumberOfCopies(7)
rotate.CopyInputOn()
mapper = vtkDataSetMapper()
mapper.SetInputConnection(rotate.GetOutputPort())
actor = vtkActor()
actor.SetMapper(mapper)
ren1 = vtkRenderer()
ren1.AddActor(actor)
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
renWin.SetSize(512,512)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
iren.Initialize()
renWin.Render()
# --- end of script --
