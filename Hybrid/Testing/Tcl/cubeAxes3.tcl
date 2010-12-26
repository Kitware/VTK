# This example illustrates how one may explicitly specify the range of each
# axes that's used to define the prop, while displaying data with a different
# set of bounds (unlike cubeAxes2.tcl). This example allows you to separate
# the notion of extent of the axes in physical space (bounds) and the extent
# of the values it represents. In other words, you can have the ticks and
# labels show a different range.
#
package require vtk
package require vtkinteraction

# read in an interesting object and outline it
#
vtkBYUReader fohe
    fohe SetGeometryFileName "$VTK_DATA_ROOT/Data/teapot.g"
vtkPolyDataNormals normals
    normals SetInputConnection [fohe GetOutputPort]
vtkPolyDataMapper foheMapper
    foheMapper SetInputConnection [normals GetOutputPort]
vtkLODActor foheActor
    foheActor SetMapper foheMapper

vtkOutlineFilter outline
    outline SetInputConnection [normals GetOutputPort]
vtkPolyDataMapper mapOutline
    mapOutline SetInputConnection [outline GetOutputPort]
vtkActor outlineActor
    outlineActor SetMapper mapOutline
    [outlineActor GetProperty] SetColor 0 0 0

# Create the RenderWindow, Renderer, and setup viewports
vtkCamera camera
    camera SetClippingRange 1.60187 20.0842
    camera SetFocalPoint 0.21406 1.5 0
    camera SetPosition 11.63 6.32 5.77
    camera SetViewUp 0.180325 0.549245 -0.815974
vtkLight light
    light SetFocalPoint 0.21406 1.5 0
    light SetPosition 8.3761 4.94858 4.12505

vtkRenderer ren2
    ren2 SetActiveCamera camera
    ren2 AddLight light
vtkRenderWindow renWin
    renWin SetMultiSamples 0
    renWin AddRenderer ren2
    renWin SetWindowName "VTK - Cube Axes custom range"
    renWin SetSize 600 600
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren2 AddViewProp foheActor
ren2 AddViewProp outlineActor
ren2 SetBackground 0.1 0.2 0.4

normals Update
set bounds [[normals GetOutput] GetBounds]

vtkCubeAxesActor axes2
    axes2 SetBounds [lindex $bounds 0] [lindex $bounds 1] [lindex $bounds 2] \
      [lindex $bounds 3] [lindex $bounds 4] [lindex $bounds 5]
    axes2 SetXAxisRange 20 300
    axes2 SetYAxisRange -0.01 0.01
    axes2 SetCamera [ren2 GetActiveCamera]
    axes2 SetXLabelFormat "%6.1f"
    axes2 SetYLabelFormat "%6.1f"
    axes2 SetZLabelFormat "%6.1f"
    axes2 SetFlyModeToClosestTriad
ren2 AddViewProp axes2
renWin Render

ren2 ResetCamera
renWin Render

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

iren Initialize

proc TkCheckAbort {} {
  set foo [renWin GetEventPending]
  if {$foo != 0} {renWin SetAbortRender 1}
}
renWin AddObserver "AbortCheckEvent" {TkCheckAbort}

# prevent the tk window from showing up then start the event loop
wm withdraw .
