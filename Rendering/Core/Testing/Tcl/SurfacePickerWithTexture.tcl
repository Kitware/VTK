package require vtk
package require vtkinteraction

#
# Do picking with an actor with a texture.
# This example draws a cone at the pick point, with the color
# of the cone set from the color of the texture at the pick position.
#

# renderer and interactor
vtkRenderer ren

vtkRenderWindow renWin
renWin AddRenderer ren

vtkRenderWindowInteractor iren
iren SetRenderWindow renWin

# read the volume
vtkJPEGReader reader
reader SetFileName "$VTK_DATA_ROOT/Data/beach.jpg"

#---------------------------------------------------------
# Do the surface rendering
vtkSphereSource sphereSource
sphereSource SetRadius 100

vtkTextureMapToSphere textureSphere
textureSphere SetInputConnection [sphereSource GetOutputPort]

vtkStripper sphereStripper
sphereStripper SetInputConnection [textureSphere GetOutputPort]
sphereStripper SetMaximumLength 5

vtkPolyDataMapper sphereMapper
sphereMapper SetInputConnection [sphereStripper GetOutputPort]
sphereMapper ScalarVisibilityOff

vtkTexture sphereTexture
sphereTexture SetInputConnection [reader GetOutputPort]

vtkProperty sphereProperty
sphereProperty BackfaceCullingOn

vtkActor sphere
sphere SetMapper sphereMapper
sphere SetTexture sphereTexture
sphere SetProperty sphereProperty

#---------------------------------------------------------
ren AddViewProp sphere

set camera [ren GetActiveCamera]
$camera SetFocalPoint 0 0 0
$camera SetPosition 100 400 -100
$camera SetViewUp 0 0 -1

ren ResetCameraClippingRange

renWin Render

#---------------------------------------------------------
# the cone should point along the Z axis
vtkConeSource coneSource
coneSource CappingOn
coneSource SetHeight 12
coneSource SetRadius 5
coneSource SetResolution 31
coneSource SetCenter 6 0 0
coneSource SetDirection -1 0 0

#---------------------------------------------------------
vtkCellPicker picker
picker SetTolerance 1e-6
picker PickTextureDataOn

# A function to point an actor along a vector
proc PointCone {actor nx ny nz} {
  if [expr $nx < 0.0] {
    $actor RotateWXYZ 180 0 1 0
    $actor RotateWXYZ 180 [expr ($nx - 1.0)*0.5] [expr $ny*0.5] [expr $nz*0.5]
  } else {
    $actor RotateWXYZ 180 [expr ($nx + 1.0)*0.5] [expr $ny*0.5] [expr $nz*0.5]
  }
}

# Pick the actor
picker Pick 104 154 0 ren
#puts [picker Print]
set p [picker GetPickPosition]
set n [picker GetPickNormal]
set ijk [picker GetPointIJK]
set data [picker GetDataSet]

set i [lindex $ijk 0]
set j [lindex $ijk 1]
set k [lindex $ijk 2]

if { [$data IsA "vtkImageData"] } {
  set r [$data GetScalarComponentAsDouble $i $j $k 0]
  set g [$data GetScalarComponentAsDouble $i $j $k 1]
  set b [$data GetScalarComponentAsDouble $i $j $k 2]
} else {
  set r 255.0
  set g 0.0
  set b 0.0
}

set r [expr $r / 255.0]
set g [expr $g / 255.0]
set b [expr $b / 255.0]

vtkActor coneActor1
coneActor1 PickableOff
vtkDataSetMapper coneMapper1
coneMapper1 SetInputConnection [coneSource GetOutputPort]
coneActor1 SetMapper coneMapper1
[coneActor1 GetProperty] SetColor $r $g $b
[coneActor1 GetProperty] BackfaceCullingOn
coneActor1 SetPosition [lindex $p 0] [lindex $p 1] [lindex $p 2]
PointCone coneActor1 [lindex $n 0] [lindex $n 1] [lindex $n 2]
ren AddViewProp coneActor1

ren ResetCameraClippingRange

renWin Render

#---------------------------------------------------------
# test-related code
proc TkCheckAbort {} {
  set foo [renWin GetEventPending]
  if {$foo != 0} {renWin SetAbortRender 1}
}
renWin AddObserver AbortCheckEvent {TkCheckAbort}

iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize

wm withdraw .

