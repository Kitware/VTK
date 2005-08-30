#
# This simple example shows how to do basic rendering and pipeline
# creation.
#
# We start off by loading some Tcl modules. One is the basic VTK library;
# the second is a package for rendering, and the last includes a set
# of color definitions.
#
package require vtk
package require vtkinteraction
package require vtktesting

# This creates a polygonal cylinder model with eight circumferential facets.
#
vtkCylinderSource cylinder
    cylinder SetResolution 8

# The mapper is responsible for pushing the geometry into the graphics
# library. It may also do color mapping, if scalars or other attributes
# are defined.
#
vtkPolyDataMapper cylinderMapper
    cylinderMapper SetInputConnection [cylinder GetOutputPort]

# The actor is a grouping mechanism: besides the geometry (mapper), it
# also has a property, transformation matrix, and/or texture map.
# Here we set its color and rotate it -22.5 degrees.
vtkActor cylinderActor
    cylinderActor SetMapper cylinderMapper
    eval [cylinderActor GetProperty] SetColor $tomato
    cylinderActor RotateX  30.0
    cylinderActor RotateY -45.0

# Create the graphics structure. The renderer renders into the 
# render window. The render window interactor captures mouse events
# and will perform appropriate camera or actor manipulation
# depending on the nature of the events.
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor cylinderActor
ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 200 200

# The next line associates a Tcl proc with a "keypress-u" event
# in the rendering window. In this case the proc deiconifies the
# .vtkInteract Tk form that was defined when we loaded
# "package require vtkinteraction".
iren AddObserver UserEvent {wm deiconify .vtkInteract}

# This allows the interactor to initalize itself. It has to be
# called before an event loop. In this example, we allow Tk to
# start the event loop (this is done automatically by Tk after
# the user script is executed).
iren Initialize

# We'll zoom in a little by accessing the camera and invoking a "Zoom"
# method on it.
ren1 ResetCamera
[ren1 GetActiveCamera] Zoom 1.5
renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .



