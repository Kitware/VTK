#!/usr/bin/env python
import vtk

# Control the resolution of the test
res = 12

# Create pipeline. Use two plane sources:
# one plane imprints on the other plane.
#
plane1 = vtk.vtkPlaneSource()
plane1.SetXResolution(res)
plane1.SetYResolution(res)
plane1.SetOrigin(0,0,0)
plane1.SetPoint1(10,0,0)
plane1.SetPoint2(0,10,0)

plane2 = vtk.vtkPlaneSource()
plane2.SetXResolution(2*res)
plane2.SetYResolution(2*res)
plane2.SetOrigin(2.25,2.25,0)
plane2.SetPoint1(7.25,2.25,0)
plane2.SetPoint2(2.25,7.25,0)

xForm = vtk.vtkTransform()
xForm.RotateZ(-25)

xFormF = vtk.vtkTransformPolyDataFilter()
xFormF.SetInputConnection(plane2.GetOutputPort())
xFormF.SetTransform(xForm)

imp = vtk.vtkImprintFilter()
imp.SetTargetConnection(plane1.GetOutputPort())
imp.SetImprintConnection(xFormF.GetOutputPort())
imp.SetTolerance(0.001)
imp.Update()

mapper = vtk.vtkPolyDataMapper()
mapper.SetInputConnection(imp.GetOutputPort())

actor = vtk.vtkActor()
actor.SetMapper(mapper)
actor.GetProperty().SetRepresentationToWireframe()

# Create the RenderWindow, Renderer
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer( ren1 )
renWin.SetSize(400,400)

iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

ren1.AddActor(actor)

renWin.Render()
iren.Start()
