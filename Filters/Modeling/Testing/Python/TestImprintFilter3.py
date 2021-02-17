#!/usr/bin/env python
import vtk

# Control the resolution of the test
res = 12

# Create pipeline. Use two plane sources:
# one plane imprints on the other plane.
#
target = vtk.vtkPlaneSource()
target.SetXResolution(res)
target.SetYResolution(res)
target.SetOrigin(0,0,0)
target.SetPoint1(10,0,0)
target.SetPoint2(0,10,0)

plane2 = vtk.vtkPlaneSource()
plane2.SetXResolution(2*res)
plane2.SetYResolution(2*res)
plane2.SetOrigin(2.25,2.25,0)
plane2.SetPoint1(7.25,2.25,0)
plane2.SetPoint2(2.25,7.25,0)

xForm = vtk.vtkTransform()
xForm.RotateZ(-25)
xForm.Translate(-1.5,1.5,0)

xFormF = vtk.vtkTransformPolyDataFilter()
xFormF.SetInputConnection(plane2.GetOutputPort())
xFormF.SetTransform(xForm)

# Output candidate target cells
imp = vtk.vtkImprintFilter()
imp.SetTargetConnection(target.GetOutputPort())
imp.SetImprintConnection(xFormF.GetOutputPort())
imp.SetTolerance(0.001)
imp.SetOutputTypeToTargetCells()
imp.Update()

targetMapper = vtk.vtkPolyDataMapper()
targetMapper.SetInputConnection(imp.GetOutputPort())

targetActor = vtk.vtkActor()
targetActor.SetMapper(targetMapper)
targetActor.GetProperty().SetRepresentationToWireframe()
targetActor.GetProperty().SetColor(0,1,0)

imprintMapper = vtk.vtkPolyDataMapper()
imprintMapper.SetInputConnection(xFormF.GetOutputPort())

imprintActor = vtk.vtkActor()
imprintActor.SetMapper(imprintMapper)
imprintActor.GetProperty().SetRepresentationToWireframe()
imprintActor.GetProperty().SetColor(1,0,0)

# Output imprinted cells
imp2 = vtk.vtkImprintFilter()
imp2.SetTargetConnection(target.GetOutputPort())
imp2.SetImprintConnection(xFormF.GetOutputPort())
imp2.SetTolerance(0.001)
imp2.SetOutputTypeToImprintedCells()
imp2.Update()

targetMapper2 = vtk.vtkPolyDataMapper()
targetMapper2.SetInputConnection(imp2.GetOutputPort())

targetActor2 = vtk.vtkActor()
targetActor2.SetMapper(targetMapper2)
targetActor2.GetProperty().SetRepresentationToWireframe()
targetActor2.GetProperty().SetColor(0,1,0)

imprintMapper2 = vtk.vtkPolyDataMapper()
imprintMapper2.SetInputConnection(xFormF.GetOutputPort())

imprintActor2 = vtk.vtkActor()
imprintActor2.SetMapper(imprintMapper2)
imprintActor2.GetProperty().SetRepresentationToWireframe()
imprintActor2.GetProperty().SetColor(1,0,0)

# Create the RenderWindow, Renderer
#
ren1 = vtk.vtkRenderer()
ren1.SetViewport(0,0,0.5,1)
ren2 = vtk.vtkRenderer()
ren2.SetViewport(0.5,0,1,1)
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer( ren1 )
renWin.AddRenderer( ren2 )
renWin.SetSize(600,300)

iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

ren1.AddActor(imprintActor)
ren1.AddActor(targetActor)

ren2.AddActor(imprintActor2)
ren2.AddActor(targetActor2)

renWin.Render()
iren.Start()
