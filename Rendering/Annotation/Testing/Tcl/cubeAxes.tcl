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
    ren1 SetViewport 0 0 0.5 1.0
    ren1 SetActiveCamera camera
    ren1 AddLight light
vtkRenderer ren2
    ren2 SetViewport 0.5 0 1.0 1.0
    ren2 SetActiveCamera camera
    ren2 AddLight light
vtkRenderWindow renWin
    renWin SetMultiSamples 0
    renWin AddRenderer ren1
    renWin AddRenderer ren2
    renWin SetWindowName "VTK - Cube Axes"
    renWin SetSize 790 400
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddViewProp foheActor
ren1 AddViewProp outlineActor
ren2 AddViewProp foheActor
ren2 AddViewProp outlineActor

ren1 SetBackground 0.1 0.2 0.4
ren2 SetBackground 0.1 0.2 0.4

vtkTextProperty tprop
    tprop SetColor 1 1 1
    tprop ShadowOn

vtkCubeAxesActor2D axes
    axes SetInputConnection [normals GetOutputPort]
    axes SetCamera [ren1 GetActiveCamera]
    axes SetLabelFormat "%6.1f"
    axes SetFlyModeToOuterEdges
    axes SetFontFactor 0.8
    axes SetAxisTitleTextProperty tprop
    axes SetAxisLabelTextProperty tprop
ren1 AddViewProp axes

vtkCubeAxesActor2D axes2
    axes2 SetViewProp foheActor
    axes2 SetCamera [ren2 GetActiveCamera]
    axes2 SetLabelFormat [axes GetLabelFormat]
    axes2 SetFlyModeToClosestTriad
    axes2 SetFontFactor [axes GetFontFactor]
    axes2 ScalingOff
    axes2 SetAxisTitleTextProperty tprop
    axes2 SetAxisLabelTextProperty tprop
ren2 AddViewProp axes2

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
