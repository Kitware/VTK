catch {load vtktcl}
#
# Demonstrate how to use picking and annotation
# get the interactor ui
source ../../examplesTcl/vtkInt.tcl

# create a sphere source and actor
#
vtkSphereSource sphere

vtkPolyDataMapper sphereMapper
    sphereMapper SetInput [sphere GetOutput]
    sphereMapper GlobalImmediateModeRenderingOn
vtkLODActor sphereActor
    sphereActor SetMapper sphereMapper

# create the spikes using a cone source and the sphere source
#
vtkConeSource cone
vtkGlyph3D glyph
    glyph SetInput [sphere GetOutput]
    glyph SetSource [cone GetOutput]
    glyph SetVectorModeToUseNormal
    glyph SetScaleModeToScaleByVector
    glyph SetScaleFactor 0.25
vtkPolyDataMapper spikeMapper
    spikeMapper SetInput [glyph GetOutput]
vtkLODActor spikeActor
    spikeActor SetMapper spikeMapper

# Picking stuff
vtkCellPicker picker
    picker SetEndPickMethod annotatePick
vtkTextMapper textMapper
    textMapper SetFontFamilyToArial
    textMapper SetFontSize 10
    textMapper BoldOn
    textMapper ShadowOn
vtkActor2D textActor
    textActor VisibilityOff
    textActor SetMapper textMapper
    [textActor GetProperty] SetColor 1 0 0

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin
    iren SetPicker picker

# Add the actors to the renderer, set the background and size
#
ren1 AddActor2D textActor
ren1 AddActor sphereActor
ren1 AddActor spikeActor
ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 300 300

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

set cam1 [ren1 GetActiveCamera]
$cam1 Zoom 1.4
iren Initialize
renWin SetFileName "annotatePick.tcl.ppm"
#renWin SaveImageAsPPM

proc TkCheckAbort {} {
  set foo [renWin GetEventPending]
  if {$foo != 0} {renWin SetAbortRender 1}
}
renWin SetAbortCheckMethod {TkCheckAbort}

# prevent the tk window from showing up then start the event loop
wm withdraw .

proc annotatePick {} {
    if { [picker GetCellId] < 0 } {
	textActor VisibilityOff

    } else {
	set selPt [picker GetSelectionPoint]
	set x [lindex $selPt 0] 
	set y [lindex $selPt 1]
	set pickPos [picker GetPickPosition]
	set xp [lindex $pickPos 0] 
	set yp [lindex $pickPos 1]
	set zp [lindex $pickPos 2]

	textMapper SetInput "($xp, $yp, $zp)"
	textActor SetPosition $x $y
	textActor VisibilityOn
    }

    renWin Render
}

picker Pick 85 126 0 ren1
