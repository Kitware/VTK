package require vtk
package require vtkinteraction

# Demonstrate how to use the vtkBoxWidget.
# This script uses a 3D box widget to define a "clipping box" to clip some
# simple geometry (a mace). Make sure that you hit the "W" key to activate the widget.

# create a sphere source 
#
vtkSphereSource sphere
vtkConeSource cone
vtkGlyph3D glyph
    glyph SetInputConnection [sphere GetOutputPort]
    glyph SetSource [cone GetOutput]
    glyph SetVectorModeToUseNormal
    glyph SetScaleModeToScaleByVector
    glyph SetScaleFactor 0.25
vtkAppendPolyData apd
    apd AddInput [glyph GetOutput]
    apd AddInput [sphere GetOutput]
vtkPolyDataMapper maceMapper
    maceMapper SetInputConnection [apd GetOutputPort]
vtkLODActor maceActor
    maceActor SetMapper maceMapper
    maceActor VisibilityOn

vtkPlanes planes
vtkClipPolyData clipper
    clipper SetInputConnection [apd GetOutputPort]
    clipper SetClipFunction planes
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
vtkBoxWidget boxWidget
    boxWidget SetInteractor iren

ren1 AddActor maceActor
ren1 AddActor selectActor

# Add the actors to the renderer, set the background and size
#
ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 300 300

# place the interactor initially
boxWidget SetInputConnection [glyph GetOutputPort]
boxWidget PlaceWidget
boxWidget AddObserver EndInteractionEvent SelectPolygons

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .

proc SelectPolygons {} {
   boxWidget GetPlanes planes
   selectActor VisibilityOn
}
