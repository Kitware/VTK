# This example demonstrates the use of vtkCubeAxesActor2D to indicate the
# position in space that the camera is currently viewing.
# The vtkCubeAxesActor2D draws axes on the bounding box of the data set and
# labels the axes with x-y-z coordinates.

#
# First we include the VTK Tcl packages which will make available
# all of the vtk commands to Tcl
#
package require vtk
package require vtkinteraction

# Create a vtkBYUReader and read in a data set.
#
vtkBYUReader fohe
    fohe SetGeometryFileName "$VTK_DATA_ROOT/Data/teapot.g"
# Create a vtkPolyDataNormals filter to calculate the normals of the data set.
vtkPolyDataNormals normals
    normals SetInputConnection [fohe GetOutputPort]
# Set up the associated mapper and actor.
vtkPolyDataMapper foheMapper
    foheMapper SetInputConnection [normals GetOutputPort]
vtkLODActor foheActor
    foheActor SetMapper foheMapper

# Create a vtkOutlineFilter to draw the bounding box of the data set.  Also
# create the associated mapper and actor.
vtkOutlineFilter outline
    outline SetInputConnection [normals GetOutputPort]
vtkPolyDataMapper mapOutline
    mapOutline SetInputConnection [outline GetOutputPort]
vtkActor outlineActor
    outlineActor SetMapper mapOutline
    [outlineActor GetProperty] SetColor 0 0 0

# Create a vtkCamera, and set the camera parameters.
vtkCamera camera
    camera SetClippingRange 1.60187 20.0842
    camera SetFocalPoint 0.21406 1.5 0
    camera SetPosition 8.3761 4.94858 4.12505
    camera SetViewUp 0.180325 0.549245 -0.815974

# Create a vtkLight, and set the light parameters.
vtkLight light
    light SetFocalPoint 0.21406 1.5 0
    light SetPosition 8.3761 4.94858 4.12505

# Create the Renderers.  Assign them the appropriate viewport coordinates,
# active camera, and light.
vtkRenderer ren1
    ren1 SetViewport 0 0 0.5 0.5
    ren1 SetActiveCamera camera
    ren1 AddLight light
vtkRenderer ren2
    ren2 SetViewport 0.5 0 1.0 0.5
    ren2 SetActiveCamera camera
    ren2 AddLight light
vtkRenderer ren3
    ren3 SetViewport 0 0.5 0.5 1.0
    ren3 SetActiveCamera camera
    ren3 AddLight light
vtkRenderer ren4
    ren4 SetViewport 0.5 0.5 1.0 1.0
    ren4 SetActiveCamera camera
    ren4 AddLight light

# Create the RenderWindow and RenderWindowInteractor.
vtkRenderWindow renWin
    renWin AddRenderer ren1
    renWin AddRenderer ren2
    renWin AddRenderer ren3
    renWin AddRenderer ren4
    renWin SetWindowName "VTK - Cube Axes"
    renWin SetSize 800 800
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, and set the background.
#
ren1 AddViewProp foheActor
ren1 AddViewProp outlineActor
ren2 AddViewProp foheActor
ren2 AddViewProp outlineActor
ren3 AddViewProp foheActor
ren3 AddViewProp outlineActor
ren4 AddViewProp foheActor
ren4 AddViewProp outlineActor

ren1 SetBackground 0.1 0.2 0.4
ren2 SetBackground 0.1 0.2 0.4
ren3 SetBackground 0.1 0.2 0.4
ren4 SetBackground 0.1 0.2 0.4

# Create a text property for both cube axes
#
vtkTextProperty tprop
    tprop SetColor 1 1 1
    tprop ShadowOn

# Create a vtkCubeAxesActor2D.  Use the outer edges of the bounding box to
# draw the axes.  Add the actor to the renderer.
vtkCubeAxesActor2D axes
    axes SetInputConnection [normals GetOutputPort]
    axes SetCamera [ren1 GetActiveCamera]
    axes SetLabelFormat "%6.4g"
    axes SetFlyModeToClosestTriad
    axes SetFontFactor 0.8
    axes SetAxisTitleTextProperty tprop
    axes SetAxisLabelTextProperty tprop
ren1 AddViewProp axes

# Create a vtkCubeAxesActor2D.  Use the closest vertex to the camera to
# determine where to draw the axes.  Add the actor to the renderer.
vtkCubeAxesActor2D axes2
    axes2 SetViewProp foheActor
    axes2 SetCamera [ren2 GetActiveCamera]
    axes2 SetLabelFormat "%6.4g"
    axes2 SetFlyModeToClosestTriad
    axes2 SetFontFactor 0.8
    axes2 ScalingOff
    axes2 SetAxisTitleTextProperty tprop
    axes2 SetAxisLabelTextProperty tprop
ren2 AddViewProp axes2

# No fly mode.
vtkCubeAxesActor2D axes3
    axes3 SetViewProp foheActor
    axes3 SetCamera [ren3 GetActiveCamera]
    axes3 SetLabelFormat "%6.4g"
    axes3 SetFlyModeToNone
    axes3 SetFontFactor 0.8
    axes3 ScalingOff
    axes3 SetAxisTitleTextProperty tprop
    axes3 SetAxisLabelTextProperty tprop
ren3 AddViewProp axes3

# No fly mode + set a custom z origin.
vtkCubeAxesActor2D axes4
    axes4 SetViewProp foheActor
    axes4 SetCamera [ren4 GetActiveCamera]
    axes4 SetLabelFormat "%6.4g"
    axes4 SetFlyModeToNone
    axes4 SetFontFactor 0.8
    axes4 ScalingOff
    axes4 SetAxisTitleTextProperty tprop
    axes4 SetAxisLabelTextProperty tprop
    axes4 SetZOrigin 0.0
    axes4 ZAxisVisibilityOff
ren4 AddViewProp axes4


# Render
renWin Render

# Set the user method (bound to key 'u')
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize

# Set up a check for aborting rendering.
proc TkCheckAbort {} {
  set foo [renWin GetEventPending]
  if {$foo != 0} {renWin SetAbortRender 1}
}
renWin AddObserver AbortCheckEvent {TkCheckAbort}

# Withdraw the default tk window.
wm withdraw .


