#!/usr/bin/env python
from vtkmodules.vtkFiltersGeometry import vtkGeometryFilter
from vtkmodules.vtkIOEnSight import vtkGenericEnSightReader
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
from vtkmodules.vtkRenderingOpenGL2 import vtkCompositePolyDataMapper2
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# create a rendering window and renderer
renderer = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(renderer)
renWin.StereoCapableWindowOn()
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# create pipeline
reader = vtkGenericEnSightReader()
reader.SetCaseFileName(VTK_DATA_ROOT + "/Data/EnSight/viga.case")

geometry = vtkGeometryFilter()
geometry.SetInputConnection(reader.GetOutputPort())

mapper = vtkCompositePolyDataMapper2()
mapper.SetInputConnection(geometry.GetOutputPort())

actor = vtkActor()
actor.SetMapper(mapper)

# render
renderer.AddActor(actor)
renWin.Render()
