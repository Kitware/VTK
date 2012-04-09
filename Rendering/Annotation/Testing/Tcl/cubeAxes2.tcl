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

vtkRenderer ren1
    ren1 SetViewport 0 0 0.33 0.5
    ren1 SetActiveCamera camera
    ren1 AddLight light
vtkRenderer ren2
    ren2 SetViewport 0.33 0 0.66 0.5
    ren2 SetActiveCamera camera
    ren2 AddLight light
vtkRenderer ren3
    ren3 SetViewport 0.66 0 1.0 0.5
    ren3 SetActiveCamera camera
    ren3 AddLight light
vtkRenderer ren4
    ren4 SetViewport 0 0.5 0.5 1.0
    ren4 SetActiveCamera camera
    ren4 AddLight light
vtkRenderer ren5
    ren5 SetViewport 0.5 0.5 1.0 1.0
    ren5 SetActiveCamera camera
    ren5 AddLight light
vtkRenderWindow renWin
    renWin SetMultiSamples 0
    renWin AddRenderer ren1
    renWin AddRenderer ren2
    renWin AddRenderer ren3
    renWin AddRenderer ren4
    renWin AddRenderer ren5
    renWin SetWindowName "VTK - Cube Axes"
    renWin SetSize 600 600
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddViewProp foheActor
ren1 AddViewProp outlineActor
ren2 AddViewProp foheActor
if { [info command "rtExMath"] == ""} {
  ren2 AddViewProp outlineActor
}
ren3 AddViewProp foheActor
ren3 AddViewProp outlineActor
ren4 AddViewProp foheActor
ren4 AddViewProp outlineActor
ren5 AddViewProp foheActor

ren1 SetBackground 0.1 0.2 0.4
ren2 SetBackground 0.1 0.2 0.4
ren3 SetBackground 0.1 0.2 0.4
ren4 SetBackground 0.1 0.2 0.4
ren5 SetBackground 0.1 0.2 0.4

normals Update
set bounds [[normals GetOutput] GetBounds]

vtkCubeAxesActor axes
    axes SetBounds [lindex $bounds 0] [lindex $bounds 1] [lindex $bounds 2] \
      [lindex $bounds 3] [lindex $bounds 4] [lindex $bounds 5]
    axes SetCamera [ren1 GetActiveCamera]
    axes SetXLabelFormat "%6.1f"
    axes SetYLabelFormat "%6.1f"
    axes SetZLabelFormat "%6.1f"
    axes SetFlyModeToOuterEdges
ren1 AddViewProp axes 

vtkCubeAxesActor axes2
    axes2 SetBounds [lindex $bounds 0] [lindex $bounds 1] [lindex $bounds 2] \
      [lindex $bounds 3] [lindex $bounds 4] [lindex $bounds 5]
    axes2 SetCamera [ren2 GetActiveCamera]
    axes2 SetXLabelFormat [axes GetXLabelFormat]
    axes2 SetYLabelFormat [axes GetYLabelFormat]
    axes2 SetZLabelFormat [axes GetZLabelFormat]
    axes2 SetFlyModeToClosestTriad
ren2 AddViewProp axes2 

vtkCubeAxesActor axes3
    axes3 SetBounds [lindex $bounds 0] [lindex $bounds 1] [lindex $bounds 2] \
      [lindex $bounds 3] [lindex $bounds 4] [lindex $bounds 5]
    axes3 SetCamera [ren2 GetActiveCamera]
    axes3 SetXLabelFormat [axes GetXLabelFormat]
    axes3 SetYLabelFormat [axes GetYLabelFormat]
    axes3 SetZLabelFormat [axes GetZLabelFormat]
    axes3 SetFlyModeToFurthestTriad
ren3 AddViewProp axes3

set bounds2 [axes3 GetBounds]
vtkCubeAxesActor axes4
    axes4 SetBounds [lindex $bounds2 0] [lindex $bounds2 1] \
      [lindex $bounds2 2] [lindex $bounds2 3] [lindex $bounds2 4] \
      [lindex $bounds2 5]
    axes4 SetCamera [ren2 GetActiveCamera]
    axes4 SetXLabelFormat [axes GetXLabelFormat]
    axes4 SetYLabelFormat [axes GetYLabelFormat]
    axes4 SetZLabelFormat [axes GetZLabelFormat]
    axes4 SetFlyModeToStaticTriad
ren4 AddViewProp axes4

vtkCubeAxesActor axes5
    axes5 SetBounds [lindex $bounds2 0] [lindex $bounds2 1] \
      [lindex $bounds2 2] [lindex $bounds2 3] [lindex $bounds2 4] \
      [lindex $bounds2 5]
    axes5 SetCamera [ren2 GetActiveCamera]
    axes5 SetXLabelFormat [axes GetXLabelFormat]
    axes5 SetYLabelFormat [axes GetYLabelFormat]
    axes5 SetZLabelFormat [axes GetZLabelFormat]
    axes5 SetFlyModeToStaticEdges
ren5 AddViewProp axes5

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

# for testing
set threshold 13
