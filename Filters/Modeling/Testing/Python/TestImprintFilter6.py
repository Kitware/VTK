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
# Also, exercise various permutations of the
# output types.

target = vtkPlaneSource()
target.SetXResolution(res)
target.SetYResolution(res)
target.SetOrigin(0,0,0)
target.SetPoint1(10,0,0)
target.SetPoint2(0,10,0)

plane2 = vtkPlaneSource()
plane2.SetXResolution(2*res)
plane2.SetYResolution(2*res)
plane2.SetOrigin(2.25,2.25,0)
plane2.SetPoint1(7.25,2.25,0)
plane2.SetPoint2(2.25,7.25,0)

xForm = vtkTransform()
xForm.RotateZ(-25)
xForm.Translate(-1.5,1.5,0)

xFormF = vtkTransformPolyDataFilter()
xFormF.SetInputConnection(plane2.GetOutputPort())
xFormF.SetTransform(xForm)

# Output candidate target cells
imp = vtkImprintFilter()
imp.SetTargetConnection(target.GetOutputPort())
imp.SetImprintConnection(xFormF.GetOutputPort())
imp.SetTolerance(0.001)
imp.SetOutputTypeToTargetCells()
imp.BoundaryEdgeInsertionOn()
imp.Update()

targetMapper = vtkPolyDataMapper()
targetMapper.SetInputConnection(imp.GetOutputPort())

targetActor = vtkActor()
targetActor.SetMapper(targetMapper)
targetActor.GetProperty().SetRepresentationToWireframe()
targetActor.GetProperty().SetColor(0,1,0)

imprintMapper = vtkPolyDataMapper()
imprintMapper.SetInputConnection(xFormF.GetOutputPort())

imprintActor = vtkActor()
imprintActor.SetMapper(imprintMapper)
imprintActor.GetProperty().SetRepresentationToWireframe()
imprintActor.GetProperty().SetColor(1,0,0)

# Output imprinted cells
imp2 = vtkImprintFilter()
imp2.SetTargetConnection(target.GetOutputPort())
imp2.SetImprintConnection(xFormF.GetOutputPort())
imp2.SetTolerance(0.001)
imp2.SetOutputTypeToImprintedCells()
imp2.BoundaryEdgeInsertionOn()
imp2.Update()

targetMapper2 = vtkPolyDataMapper()
targetMapper2.SetInputConnection(imp2.GetOutputPort())

targetActor2 = vtkActor()
targetActor2.SetMapper(targetMapper2)
targetActor2.GetProperty().SetRepresentationToWireframe()
targetActor2.GetProperty().SetColor(0,1,0)

imprintMapper2 = vtkPolyDataMapper()
imprintMapper2.SetInputConnection(xFormF.GetOutputPort())

imprintActor2 = vtkActor()
imprintActor2.SetMapper(imprintMapper2)
imprintActor2.GetProperty().SetRepresentationToWireframe()
imprintActor2.GetProperty().SetColor(1,0,0)

# Output imprint with points projected onto target.
imp3 = vtkImprintFilter()
imp3.SetTargetConnection(target.GetOutputPort())
imp3.SetImprintConnection(xFormF.GetOutputPort())
imp3.SetTolerance(0.001)
imp3.SetOutputTypeToProjectedImprint()
imp3.BoundaryEdgeInsertionOn()
imp3.Update()

imprintMapper3 = vtkPolyDataMapper()
imprintMapper3.SetInputConnection(imp3.GetOutputPort())

imprintActor3 = vtkActor()
imprintActor3.SetMapper(imprintMapper3)
imprintActor3.GetProperty().SetRepresentationToWireframe()
imprintActor3.GetProperty().SetColor(1,0,0)

# Output the imprinted region
imp4 = vtkImprintFilter()
imp4.SetTargetConnection(target.GetOutputPort())
imp4.SetImprintConnection(xFormF.GetOutputPort())
imp4.SetTolerance(0.001)
imp4.SetOutputTypeToImprintedRegion()
imp4.BoundaryEdgeInsertionOn()
imp4.Update()

imprintMapper4 = vtkPolyDataMapper()
imprintMapper4.SetInputConnection(imp4.GetOutputPort())

imprintActor4 = vtkActor()
imprintActor4.SetMapper(imprintMapper4)
imprintActor4.GetProperty().SetRepresentationToWireframe()
imprintActor4.GetProperty().SetColor(1,0,0)

# Create the RenderWindow, Renderer
#
ren1 = vtkRenderer()
ren1.SetViewport(0,0,0.5,0.5)
ren2 = vtkRenderer()
ren2.SetViewport(0.5,0,1,0.5)
ren3 = vtkRenderer()
ren3.SetViewport(0,0.5,0.5,1.0)
ren4 = vtkRenderer()
ren4.SetViewport(0.5,0.5,1,1)

renWin = vtkRenderWindow()
renWin.AddRenderer( ren1 )
renWin.AddRenderer( ren2 )
renWin.AddRenderer( ren3 )
renWin.AddRenderer( ren4 )
renWin.SetSize(400,400)

iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

ren1.AddActor(imprintActor)
ren1.AddActor(targetActor)

ren2.AddActor(imprintActor2)
ren2.AddActor(targetActor2)

ren3.AddActor(imprintActor3)

ren4.AddActor(imprintActor4)

renWin.Render()
iren.Start()
