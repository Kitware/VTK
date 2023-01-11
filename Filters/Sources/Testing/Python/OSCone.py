#!/usr/bin/env python
from vtkmodules.vtkFiltersSources import vtkConeSource
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderer,
)
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

renWin = vtkRenderWindow()
renWin.OffScreenRenderingOn()
renWin.SetMultiSamples(0)
ren = vtkRenderer()
renWin.AddRenderer(ren)
cone = vtkConeSource()
mp = vtkPolyDataMapper()
mp.SetInputConnection(cone.GetOutputPort())
actor = vtkActor()
actor.SetMapper(mp)
ren.AddActor(actor)
renWin.Render()
# prevent the tk window from showing up then start the event loop
# --- end of script --
