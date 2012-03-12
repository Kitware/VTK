package require vtk
package require vtkinteraction

#
# Do picking with a locator to speed things up
#

# renderer and interactor
vtkRenderer ren

vtkRenderWindow renWin
renWin AddRenderer ren

vtkRenderWindowInteractor iren
iren SetRenderWindow renWin

# read the volume
vtkVolume16Reader v16
v16 SetDataDimensions 64 64
v16 SetImageRange 1 93
v16 SetDataByteOrderToLittleEndian
v16 SetFilePrefix "$VTK_DATA_ROOT/Data/headsq/quarter"
v16 SetDataSpacing 3.2 3.2 1.5

#---------------------------------------------------------
# Do the surface rendering
vtkMarchingCubes boneExtractor
boneExtractor SetInputConnection [v16 GetOutputPort]
boneExtractor SetValue 0 1150

vtkPolyDataNormals boneNormals
boneNormals SetInputConnection [boneExtractor GetOutputPort]
boneNormals SetFeatureAngle 60.0

vtkStripper boneStripper
boneStripper SetInputConnection [boneNormals GetOutputPort]
boneStripper SetMaximumLength 5

vtkCellLocator boneLocator
boneLocator SetDataSet [boneStripper GetOutput]
boneLocator LazyEvaluationOn

vtkPolyDataMapper boneMapper
boneMapper SetInputConnection [boneStripper GetOutputPort]
boneMapper ScalarVisibilityOff

vtkProperty boneProperty
boneProperty SetColor 1.0 1.0 0.9

vtkActor bone
bone SetMapper boneMapper
bone SetProperty boneProperty

#---------------------------------------------------------
# Create an image actor

vtkLookupTable table
table SetRange 0 2000
table SetRampToLinear
table SetValueRange 0 1
table SetHueRange 0 0
table SetSaturationRange 0 0

vtkImageMapToColors mapToColors
mapToColors SetInputConnection [v16 GetOutputPort]
mapToColors SetLookupTable table


vtkImageActor imageActor
[imageActor GetMapper] SetInputConnection [mapToColors GetOutputPort]
imageActor SetDisplayExtent 32 32 0 63 0 92

#---------------------------------------------------------
# make a clipping plane

set cx 100.8
set cy 100.8
set cz  69.0

vtkPlane boneClip
boneClip SetNormal 0 1 0
boneClip SetOrigin $cx $cy $cz

boneMapper AddClippingPlane boneClip

#---------------------------------------------------------
ren AddViewProp bone
ren AddViewProp imageActor

set camera [ren GetActiveCamera]
$camera SetFocalPoint $cx $cy $cz
$camera SetPosition [expr $cx + 400] [expr $cy + 100] [expr $cz - 100]
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
picker AddLocator boneLocator

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
picker Pick 70 120 0 ren
#puts [picker Print]
set p [picker GetPickPosition]
set n [picker GetPickNormal]

vtkActor coneActor1
coneActor1 PickableOff
vtkDataSetMapper coneMapper1
coneMapper1 SetInputConnection [coneSource GetOutputPort]
coneActor1 SetMapper coneMapper1
[coneActor1 GetProperty] SetColor 1 0 0
coneActor1 SetPosition [lindex $p 0] [lindex $p 1] [lindex $p 2]
PointCone coneActor1 [lindex $n 0] [lindex $n 1] [lindex $n 2]
ren AddViewProp coneActor1

# Pick the image
picker Pick 170 220 0 ren
#puts [picker Print]
set p [picker GetPickPosition]
set n [picker GetPickNormal]

vtkActor coneActor2
coneActor2 PickableOff
vtkDataSetMapper coneMapper2
coneMapper2 SetInputConnection [coneSource GetOutputPort]
coneActor2 SetMapper coneMapper2
[coneActor2 GetProperty] SetColor 1 0 0
coneActor2 SetPosition [lindex $p 0] [lindex $p 1] [lindex $p 2]
PointCone coneActor2 [lindex $n 0] [lindex $n 1] [lindex $n 2]
ren AddViewProp coneActor2

# Pick the actor again, in a way that the ray gets clipped
picker Pick 180 220 0 ren
set p [picker GetPickPosition]
set n [picker GetPickNormal]

vtkActor coneActor3
coneActor3 PickableOff
vtkDataSetMapper coneMapper3
coneMapper3 SetInputConnection [coneSource GetOutputPort]
coneActor3 SetMapper coneMapper3
[coneActor3 GetProperty] SetColor 1 0 0
coneActor3 SetPosition [lindex $p 0] [lindex $p 1] [lindex $p 2]
PointCone coneActor3 [lindex $n 0] [lindex $n 1] [lindex $n 2]
ren AddViewProp coneActor3

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



