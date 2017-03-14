#!/usr/bin/env python

# Demonstrate how to use the vtkBoxWidget 3D widget. This script uses
# a 3D box widget to define a "clipping box" to clip some simple
# geometry (a mace). Make sure that you hit the "i" key to activate
# the widget.

import vtk

# Create a mace out of filters.
sphere = vtk.vtkSphereSource()
cone = vtk.vtkConeSource()
glyph = vtk.vtkGlyph3D()
glyph.SetInputConnection(sphere.GetOutputPort())
glyph.SetSourceConnection(cone.GetOutputPort())
glyph.SetVectorModeToUseNormal()
glyph.SetScaleModeToScaleByVector()
glyph.SetScaleFactor(0.25)

# The sphere and spikes are appended into a single polydata. This just
# makes things simpler to manage.
apd = vtk.vtkAppendPolyData()
apd.AddInputConnection(glyph.GetOutputPort())
apd.AddInputConnection(sphere.GetOutputPort())
maceMapper = vtk.vtkPolyDataMapper()
maceMapper.SetInputConnection(apd.GetOutputPort())
maceActor = vtk.vtkLODActor()
maceActor.SetMapper(maceMapper)
maceActor.VisibilityOn()

# This portion of the code clips the mace with the vtkPlanes implicit
# function.  The clipped region is colored green.
planes = vtk.vtkPlanes()
clipper = vtk.vtkClipPolyData()
clipper.SetInputConnection(apd.GetOutputPort())
clipper.SetClipFunction(planes)
clipper.InsideOutOn()
selectMapper = vtk.vtkPolyDataMapper()
selectMapper.SetInputConnection(clipper.GetOutputPort())
selectActor = vtk.vtkLODActor()
selectActor.SetMapper(selectMapper)
selectActor.GetProperty().SetColor(0, 1, 0)
selectActor.VisibilityOff()
selectActor.SetScale(1.01, 1.01, 1.01)

# Create the RenderWindow, Renderer and both Actors
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren)

iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# The SetInteractor method is how 3D widgets are associated with the
# render window interactor.  Internally, SetInteractor sets up a bunch
# of callbacks using the Command/Observer mechanism (AddObserver()).
boxWidget = vtk.vtkBoxWidget()
boxWidget.SetInteractor(iren)
boxWidget.SetPlaceFactor(1.25)

ren.AddActor(maceActor)
ren.AddActor(selectActor)

# Add the actors to the renderer, set the background and size
ren.SetBackground(0.1, 0.2, 0.4)
renWin.SetSize(300, 300)

# This callback function does the actual work: updates the vtkPlanes
# implicit function.  This in turn causes the pipeline to update.
def SelectPolygons(object, event):
    # object will be the boxWidget
    global selectActor, planes
    object.GetPlanes(planes)
    selectActor.VisibilityOn()

glyph.Update()
# Place the interactor initially. The input to a 3D widget is used to
# initially position and scale the widget. The "EndInteractionEvent" is
# observed which invokes the SelectPolygons callback.
boxWidget.SetInputConnection(glyph.GetOutputPort())
boxWidget.PlaceWidget()
boxWidget.AddObserver("EndInteractionEvent", SelectPolygons)

iren.Initialize()
renWin.Render()
iren.Start()
