#!/usr/bin/env python
from vtkmodules.vtkCommonTransforms import vtkTransform
from vtkmodules.vtkFiltersGeneral import vtkTransformPolyDataFilter
from vtkmodules.vtkFiltersModeling import vtkImprintFilter
from vtkmodules.vtkFiltersSources import vtkPlaneSource
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

# Control the resolution of the test
res = 12

# Create pipeline. Use two plane sources:
# one plane imprints on the other plane.
#
plane1 = vtkPlaneSource()
plane1.SetXResolution(res)
plane1.SetYResolution(res)
plane1.SetOrigin(0,0,0)
plane1.SetPoint1(10,0,0)
plane1.SetPoint2(0,10,0)

plane2 = vtkPlaneSource()
plane2.SetXResolution(2*res)
plane2.SetYResolution(2*res)
plane2.SetOrigin(2.25,2.25,0)
plane2.SetPoint1(7.25,2.25,0)
plane2.SetPoint2(2.25,7.25,0)

xForm = vtkTransform()
xForm.RotateZ(-25)

xFormF = vtkTransformPolyDataFilter()
xFormF.SetInputConnection(plane2.GetOutputPort())
xFormF.SetTransform(xForm)

imp = vtkImprintFilter()
imp.SetTargetConnection(plane1.GetOutputPort())
imp.SetImprintConnection(xFormF.GetOutputPort())
imp.SetTolerance(0.001)
imp.Update()

mapper = vtkPolyDataMapper()
mapper.SetInputConnection(imp.GetOutputPort())

actor = vtkActor()
actor.SetMapper(mapper)
actor.GetProperty().SetRepresentationToWireframe()

# Create the RenderWindow, Renderer
#
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer( ren1 )
renWin.SetSize(400,400)

iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

ren1.AddActor(actor)

renWin.Render()
iren.Start()
