#!/usr/bin/env python
import vtk

# A simpler imprint test. One plane
# imprints a second plane.

# Control the resolution of the test
res = 2

# Create the RenderWindow, Renderer
#
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer( ren )

iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Use two plane sources:
# one plane imprints on the other plane.
#
plane1 = vtk.vtkPlaneSource()
plane1.SetXResolution(3)
plane1.SetYResolution(1)
plane1.SetOrigin(0,0,0)
plane1.SetPoint1(2,0,0)
plane1.SetPoint2(0,1,0)

p1Mapper = vtk.vtkPolyDataMapper()
p1Mapper.SetInputConnection(plane1.GetOutputPort())

p1Actor = vtk.vtkActor()
p1Actor.SetMapper(p1Mapper)
p1Actor.GetProperty().SetRepresentationToSurface()

plane2 = vtk.vtkPlaneSource()
plane2.SetXResolution(res)
plane2.SetYResolution(res)
plane2.SetOrigin(-0.25,0.25,0)
plane2.SetPoint1(1.5,0.25,0)
plane2.SetPoint2(-0.25,0.75,0)

p2Mapper = vtk.vtkPolyDataMapper()
p2Mapper.SetInputConnection(plane2.GetOutputPort())

p2Actor = vtk.vtkActor()
p2Actor.SetMapper(p2Mapper)
p2Actor.GetProperty().SetRepresentationToSurface()

# Now imprint
imp = vtk.vtkImprintFilter()
imp.SetTargetConnection(plane1.GetOutputPort())
imp.SetImprintConnection(plane2.GetOutputPort())
imp.SetTolerance(0.00001)
imp.Update()

mapper = vtk.vtkPolyDataMapper()
mapper.SetInputConnection(imp.GetOutputPort())

actor = vtk.vtkActor()
actor.SetMapper(mapper)
actor.GetProperty().SetRepresentationToSurface()

# Create the RenderWindow, Renderer
#
ren1 = vtk.vtkRenderer()
ren1.SetBackground(0.1,0.2,0.4)
ren1.SetViewport(0,0,0.5,1.0)
ren2 = vtk.vtkRenderer()
ren2.SetBackground(0.1,0.2,0.4)
ren2.SetViewport(0.5,0,1.0,1.0)
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer( ren1 )
renWin.AddRenderer( ren2 )
renWin.SetSize(600,300)

iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

ren1.AddActor(p1Actor)
ren1.AddActor(p2Actor)
ren1.ResetCamera()

ren2.AddActor(actor)
ren2.SetActiveCamera(ren1.GetActiveCamera())

renWin.Render()
iren.Start()
