package require vtktcl_interactor
package require vtktcl_colors


vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# create a sphere source and actor
#
vtkSphereSource sphere
vtkPolyDataMapper   sphereMapper
    sphereMapper SetInput [sphere GetOutput]
vtkLODActor sphereActor
    sphereActor SetMapper sphereMapper
eval [sphereActor GetProperty] SetDiffuseColor $banana
eval [sphereActor GetProperty] SetSpecular .4
eval [sphereActor GetProperty] SetSpecularPower 20

# create the spikes using a cone source and the sphere source
#
vtkConeSource cone
  cone SetResolution 20
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
eval [spikeActor GetProperty] SetDiffuseColor $tomato
eval [spikeActor GetProperty] SetSpecular .4
eval [spikeActor GetProperty] SetSpecularPower 20

# Add the actors to the renderer, set the background and size
#
ren1 AddActor sphereActor
ren1 AddActor spikeActor
ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 300 300

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}
set cam1 [ren1 GetActiveCamera]
$cam1 Zoom 1.4
$cam1 Azimuth 30
$cam1 Elevation 30
renWin Render
# prevent the tk window from showing up then start the event loop
wm withdraw .

vtkMath math
proc randint {min max} {
    set f [math Random $min $max]
    return [expr int($f)]
}

vtkInteractorStyleSwitch inStyle
iren SetInteractorStyle inStyle
iren SetDesiredUpdateRate .00001
iren Initialize

set actions "OnLeftButtonDown OnLeftButtonUp OnRightButtonDown OnRightButtonUp OnMiddleButtonDown OnMiddleButtonUp"
set ctrls "0 1"
set shifts "0 1"

foreach action $actions {
  foreach ctrl $ctrls {
      foreach shift $shifts {
	  inStyle $action $ctrl $shift [randint 125 175] [randint 125 175]
	  inStyle OnTimer
	  inStyle OnMouseMove $ctrl $shift [randint 125 175] [randint 125 175]
	  inStyle OnTimer
      }
  }
}
renWin Render
