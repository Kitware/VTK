package require vtk
package require vtkinteraction
package require vtktesting

# This example demonstrates how to use the vtkPlaneWidget to probe
# a dataset and then generate contours on the probed data.

# Create a mace out of filters.
#
vtkSphereSource sphere
vtkConeSource cone
vtkGlyph3D glyph
    glyph SetInputConnection [sphere GetOutputPort]
    glyph SetSourceConnection [cone GetOutputPort]
    glyph SetVectorModeToUseNormal
    glyph SetScaleModeToScaleByVector
    glyph SetScaleFactor 0.25

# The sphere and spikes are appended into a single polydata.
# This just makes things simpler to manage.
vtkAppendPolyData apd
    apd AddInputConnection [glyph GetOutputPort]
    apd AddInputConnection [sphere GetOutputPort]

vtkPolyDataMapper maceMapper
maceMapper SetInputConnection [apd GetOutputPort]

vtkLODActor maceActor
    maceActor SetMapper maceMapper
    maceActor VisibilityOn

# This portion of the code clips the mace with the vtkPlanes
# implicit function. The clipped region is colored green.
vtkPlane plane
vtkClipPolyData clipper
    clipper SetInputConnection [apd GetOutputPort]
    clipper SetClipFunction plane
    clipper InsideOutOn

vtkPolyDataMapper selectMapper
    selectMapper SetInputConnection [clipper GetOutputPort]

vtkLODActor selectActor
    selectActor SetMapper selectMapper
    [selectActor GetProperty] SetColor 0 1 0
    selectActor VisibilityOff
    selectActor SetScale 1.01 1.01 1.01

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Associate the line widget with the interactor
vtkImplicitPlaneWidget planeWidget
  planeWidget SetInteractor iren
  planeWidget SetPlaceFactor 1.25
  planeWidget SetInputConnection [glyph GetOutputPort]
  planeWidget PlaceWidget
  planeWidget AddObserver InteractionEvent myCallback

ren1 AddActor maceActor
ren1 AddActor selectActor

# Add the actors to the renderer, set the background and size
#
ren1 SetBackground 1 1 1
renWin SetSize 300 300
ren1 SetBackground 0.1 0.2 0.4

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}
renWin Render

# Prevent the tk window from showing up then start the event loop.
wm withdraw .

proc myCallback {} {
    planeWidget GetPlane plane
    selectActor VisibilityOn
}
