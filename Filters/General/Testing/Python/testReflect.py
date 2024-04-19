#!/usr/bin/env python
from vtkmodules.vtkFiltersGeneral import vtkReflectionFilter
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
reflect = vtkReflectionFilter()
reflect.SetInputConnection(cone.GetOutputPort())
reflect.SetPlaneToXMax()
reflect2 = vtkReflectionFilter()
reflect2.SetInputConnection(reflect.GetOutputPort())
reflect2.SetPlaneToYMax()
reflect3 = vtkReflectionFilter()
reflect3.SetInputConnection(reflect2.GetOutputPort())
reflect3.SetPlaneToZMax()
mapper = vtkDataSetMapper()
mapper.SetInputConnection(reflect3.GetOutputPort())
actor = vtkActor()
actor.SetMapper(mapper)
ren1 = vtkRenderer()
ren1.AddActor(actor)
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
renWin.SetSize(200,200)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
iren.Initialize()
renWin.Render()
# --- end of script --
