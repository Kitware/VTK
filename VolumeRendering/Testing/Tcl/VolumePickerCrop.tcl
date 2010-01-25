package require vtk
package require vtkinteraction

# volume render a medical data set

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
# set up the volume rendering

vtkVolumeRayCastCompositeFunction rayCastFunction

vtkVolumeRayCastMapper volumeMapper
volumeMapper SetInput [v16 GetOutput]
volumeMapper SetVolumeRayCastFunction rayCastFunction
volumeMapper CroppingOn
volumeMapper SetCroppingRegionPlanes 0.0 141.6 0.0 201.6 0.0 138.0

vtkColorTransferFunction volumeColor
volumeColor AddRGBPoint 0    0.0 0.0 0.0
volumeColor AddRGBPoint 180  0.3 0.1 0.2
volumeColor AddRGBPoint 1000 1.0 0.7 0.6
volumeColor AddRGBPoint 2000 1.0 1.0 0.9

vtkPiecewiseFunction volumeScalarOpacity
volumeScalarOpacity AddPoint 0    0.0
volumeScalarOpacity AddPoint 180  0.0
volumeScalarOpacity AddPoint 1000 0.2
volumeScalarOpacity AddPoint 2000 0.8

vtkPiecewiseFunction volumeGradientOpacity
volumeGradientOpacity AddPoint 0   0.0
volumeGradientOpacity AddPoint 90  0.5
volumeGradientOpacity AddPoint 100 1.0

vtkVolumeProperty volumeProperty
volumeProperty SetColor volumeColor
volumeProperty SetScalarOpacity volumeScalarOpacity
volumeProperty SetGradientOpacity volumeGradientOpacity
volumeProperty SetInterpolationTypeToLinear
volumeProperty ShadeOn
volumeProperty SetAmbient 0.6
volumeProperty SetDiffuse 0.6
volumeProperty SetSpecular 0.1

vtkVolume volume
volume SetMapper volumeMapper
volume SetProperty volumeProperty

#---------------------------------------------------------
# make a transform and some clipping planes

vtkTransform transform
transform RotateWXYZ -20 0.0 -0.7 0.7

volume SetUserTransform transform

set c [volume GetCenter]

vtkPlane volumeClip
volumeClip SetNormal 0 1 0
volumeClip SetOrigin [lindex $c 0] [lindex $c 1] [lindex $c 2]

volumeMapper AddClippingPlane volumeClip

#---------------------------------------------------------
ren AddViewProp volume

set camera [ren GetActiveCamera]
$camera SetFocalPoint [lindex $c 0] [lindex $c 1] [lindex $c 2]
$camera SetPosition [expr [lindex $c 0] + 500] [expr [lindex $c 1] - 100] [expr [lindex $c 2] - 100]
$camera SetViewUp 0 0 -1

ren ResetCameraClippingRange

renWin Render

#---------------------------------------------------------
# The cone source points along the x axis
vtkConeSource coneSource
coneSource CappingOn
coneSource SetHeight 12
coneSource SetRadius 5
coneSource SetResolution 31
coneSource SetCenter 6 0 0
coneSource SetDirection -1 0 0

#---------------------------------------------------------
vtkVolumePicker picker
picker SetTolerance 1e-6
picker SetVolumeOpacityIsovalue 0.3

# A function to point an actor along a vector
proc PointCone {actor nx ny nz} {
  if [expr $nx < 0.0] {
    $actor RotateWXYZ 180 0 1 0
    $actor RotateWXYZ 180 [expr ($nx - 1.0)*0.5] [expr $ny*0.5] [expr $nz*0.5]
  } else {
    $actor RotateWXYZ 180 [expr ($nx + 1.0)*0.5] [expr $ny*0.5] [expr $nz*0.5]
  }
}

# Pick part of the volume that is clipped away
picker Pick 192 103 0 ren
#puts [picker Print]
set p [picker GetPickPosition]
set n [picker GetPickNormal]

vtkActor coneActor1
coneActor1 PickableOff
vtkDataSetMapper coneMapper1
coneMapper1 SetInputConnection [coneSource GetOutputPort]
coneActor1 SetMapper coneMapper1
[coneActor1 GetProperty] SetColor 1 0 0
[coneActor1 GetProperty] BackfaceCullingOn
coneActor1 SetPosition [lindex $p 0] [lindex $p 1] [lindex $p 2]
PointCone coneActor1 [lindex $n 0] [lindex $n 1] [lindex $n 2]
ren AddViewProp coneActor1

# Pick through a cropping plane to some bone

# This should usually be left alone, but is used here to increase coverage
picker UseVolumeGradientOpacityOn
picker Pick 90 180 0 ren
set p [picker GetPickPosition]
set n [picker GetPickNormal]

vtkActor coneActor2
coneActor2 PickableOff
vtkDataSetMapper coneMapper2
coneMapper2 SetInputConnection [coneSource GetOutputPort]
coneActor2 SetMapper coneMapper2
[coneActor2 GetProperty] SetColor 1 0 0
[coneActor2 GetProperty] BackfaceCullingOn
coneActor2 SetPosition [lindex $p 0] [lindex $p 1] [lindex $p 2]
PointCone coneActor2 [lindex $n 0] [lindex $n 1] [lindex $n 2]
ren AddViewProp coneActor2

# Pick through a cropping plane to some transparent tissue

# Ignore gradient opacity, since it makes it harder to find isosurface
picker UseVolumeGradientOpacityOff
picker Pick 125 195 0 ren
set p [picker GetPickPosition]
set n [picker GetPickNormal]

vtkActor coneActor3
coneActor3 PickableOff
vtkDataSetMapper coneMapper3
coneMapper3 SetInputConnection [coneSource GetOutputPort]
coneActor3 SetMapper coneMapper3
[coneActor3 GetProperty] SetColor 1 0 0
[coneActor3 GetProperty] BackfaceCullingOn
coneActor3 SetPosition [lindex $p 0] [lindex $p 1] [lindex $p 2]
PointCone coneActor3 [lindex $n 0] [lindex $n 1] [lindex $n 2]
ren AddViewProp coneActor3

# Pick through a clipping plane
picker Pick 150 160 0 ren
set p [picker GetPickPosition]
set n [picker GetPickNormal]

vtkActor coneActor4
coneActor4 PickableOff
vtkDataSetMapper coneMapper4
coneMapper4 SetInputConnection [coneSource GetOutputPort]
coneActor4 SetMapper coneMapper4
[coneActor4 GetProperty] SetColor 1 0 0
[coneActor4 GetProperty] BackfaceCullingOn
coneActor4 SetPosition [lindex $p 0] [lindex $p 1] [lindex $p 2]
PointCone coneActor4 [lindex $n 0] [lindex $n 1] [lindex $n 2]
ren AddViewProp coneActor4

# Pick through a cropping plane with PickCroppingPlanesOn
picker PickCroppingPlanesOn
picker Pick 125 195 0 ren
set p [picker GetPickPosition]
set n [picker GetPickNormal]

vtkActor coneActor5
coneActor5 PickableOff
vtkDataSetMapper coneMapper5
coneMapper5 SetInputConnection [coneSource GetOutputPort]
coneActor5 SetMapper coneMapper5
[coneActor5 GetProperty] SetColor 0 1 0
[coneActor5 GetProperty] BackfaceCullingOn
coneActor5 SetPosition [lindex $p 0] [lindex $p 1] [lindex $p 2]
PointCone coneActor5 [lindex $n 0] [lindex $n 1] [lindex $n 2]
ren AddViewProp coneActor5

# Pick through a clipping plane with PickCroppingPlanesOn
picker PickCroppingPlanesOn
picker Pick 150 160 0 ren
set p [picker GetPickPosition]
set n [picker GetPickNormal]

vtkActor coneActor6
coneActor6 PickableOff
vtkDataSetMapper coneMapper6
coneMapper6 SetInputConnection [coneSource GetOutputPort]
coneActor6 SetMapper coneMapper6
[coneActor6 GetProperty] SetColor 0 1 0
[coneActor6 GetProperty] BackfaceCullingOn
coneActor6 SetPosition [lindex $p 0] [lindex $p 1] [lindex $p 2]
PointCone coneActor6 [lindex $n 0] [lindex $n 1] [lindex $n 2]
ren AddViewProp coneActor6

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



