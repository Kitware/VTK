package require vtk
package require vtkinteraction

# Demonstrate how to use the vtkBoxWidget to translate, scale, and rotate actors.
# The basic idea is that the box widget controls an actor's transform. A callback
# which modifies the transform is invoked as the box widget is manipulated.

# Start by creating some simple geometry; in this case a mace.
vtkSphereSource sphere
vtkConeSource cone
vtkGlyph3D glyph
    glyph SetInputConnection [sphere GetOutputPort]
    glyph SetSource [cone GetOutput]
    glyph SetVectorModeToUseNormal
    glyph SetScaleModeToScaleByVector
    glyph SetScaleFactor 0.25
vtkAppendPolyData appendData
    appendData AddInput [glyph GetOutput]
    appendData AddInput [sphere GetOutput]
vtkPolyDataMapper maceMapper
    maceMapper SetInputConnection [appendData GetOutputPort]
vtkLODActor maceActor
    maceActor SetMapper maceMapper
    maceActor VisibilityOn

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# The box widget observes the events invoked by the render window interactor.
# These events come from user interaction in the render window.
vtkBoxWidget boxWidget
    boxWidget SetInteractor iren
    boxWidget SetPlaceFactor 1.25

# Add the actors to the renderer, set the background and window size.
#
ren1 AddActor maceActor
ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 300 300

# Place the interactor initially. The actor is used to place and scale
# the interactor. An observer is added to the box widget to watch for
# interaction events. This event is captured and used to set the 
# transformation matrix of the actor.
boxWidget SetProp3D maceActor
boxWidget PlaceWidget
boxWidget AddObserver InteractionEvent TransformActor

# Support the "u" keypress user event (pops up a Tcl interpreter).
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .

# As the box widget is interacted with, it produces a transformation 
# matrix that is set on the actor.
vtkTransform t
proc TransformActor {} {
   boxWidget GetTransform t
   maceActor SetUserTransform t
}
