#!/usr/bin/env python
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

# A simpler imprint test. One plane
# imprints a second plane.

# Control the resolution of the test
res = 2

# Create the RenderWindow, Renderer
#
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer( ren )

iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Use two plane sources:
# one plane imprints on the other plane.
#
plane1 = vtkPlaneSource()
plane1.SetXResolution(3)
plane1.SetYResolution(1)
plane1.SetOrigin(0,0,0)
plane1.SetPoint1(2,0,0)
plane1.SetPoint2(0,1,0)

p1Mapper = vtkPolyDataMapper()
p1Mapper.SetInputConnection(plane1.GetOutputPort())

p1Actor = vtkActor()
p1Actor.SetMapper(p1Mapper)
p1Actor.GetProperty().SetRepresentationToSurface()

plane2 = vtkPlaneSource()
plane2.SetXResolution(res)
plane2.SetYResolution(res)
plane2.SetOrigin(-0.25,0.25,0)
plane2.SetPoint1(1.5,0.25,0)
plane2.SetPoint2(-0.25,0.75,0)

p2Mapper = vtkPolyDataMapper()
p2Mapper.SetInputConnection(plane2.GetOutputPort())

p2Actor = vtkActor()
p2Actor.SetMapper(p2Mapper)
p2Actor.GetProperty().SetRepresentationToSurface()

# Now imprint
imp = vtkImprintFilter()
imp.SetTargetConnection(plane1.GetOutputPort())
imp.SetImprintConnection(plane2.GetOutputPort())
imp.SetTolerance(0.00001)
imp.Update()

mapper = vtkPolyDataMapper()
mapper.SetInputConnection(imp.GetOutputPort())

actor = vtkActor()
actor.SetMapper(mapper)
actor.GetProperty().SetRepresentationToSurface()

# Create the RenderWindow, Renderer
#
ren1 = vtkRenderer()
ren1.SetBackground(0.1,0.2,0.4)
ren1.SetViewport(0,0,0.5,1.0)
ren2 = vtkRenderer()
ren2.SetBackground(0.1,0.2,0.4)
ren2.SetViewport(0.5,0,1.0,1.0)
renWin = vtkRenderWindow()
renWin.AddRenderer( ren1 )
renWin.AddRenderer( ren2 )
renWin.SetSize(600,300)

iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

ren1.AddActor(p1Actor)
ren1.AddActor(p2Actor)
ren1.ResetCamera()

ren2.AddActor(actor)
ren2.SetActiveCamera(ren1.GetActiveCamera())

renWin.Render()
iren.Start()
