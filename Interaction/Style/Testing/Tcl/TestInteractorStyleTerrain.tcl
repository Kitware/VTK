package require vtk
package require vtkinteraction

set Scale 5
vtkLookupTable lut
  lut SetHueRange 0.6 0
  lut SetSaturationRange 1.0 0
  lut SetValueRange 0.5 1.0

vtkDEMReader demModel
  demModel SetFileName "$VTK_DATA_ROOT/Data/SainteHelens.dem"
  demModel Update

demModel Print

set lo [expr $Scale * [lindex [demModel GetElevationBounds] 0]]
set hi [expr $Scale * [lindex [demModel GetElevationBounds] 1]]

vtkLODActor demActor

# create a pipeline for each lod mapper
vtkImageShrink3D shrink16
  shrink16 SetShrinkFactors 16 16 1
  shrink16 SetInputConnection [demModel GetOutputPort]
  shrink16 AveragingOn

vtkImageDataGeometryFilter geom16
  geom16 SetInputConnection [shrink16 GetOutputPort]
  geom16 ReleaseDataFlagOn

vtkWarpScalar warp16
  warp16 SetInputConnection [geom16 GetOutputPort]
  warp16 SetNormal 0 0 1
  warp16 UseNormalOn
  warp16 SetScaleFactor $Scale
  warp16 ReleaseDataFlagOn

vtkElevationFilter elevation16
  elevation16 SetInputConnection [warp16 GetOutputPort]
  elevation16 SetLowPoint 0 0 $lo
  elevation16 SetHighPoint 0 0 $hi
eval elevation16 SetScalarRange $lo $hi
  elevation16 ReleaseDataFlagOn

vtkPolyDataNormals normals16
  normals16 SetInputConnection [elevation16 GetOutputPort]
  normals16 SetFeatureAngle 60
  normals16 ConsistencyOff
  normals16 SplittingOff
  normals16 ReleaseDataFlagOn

vtkPolyDataMapper demMapper16
  demMapper16 SetInputConnection [normals16 GetOutputPort]
  eval demMapper16 SetScalarRange $lo $hi
  demMapper16 SetLookupTable lut
  demMapper16 ImmediateModeRenderingOn

demMapper16 Update
demActor AddLODMapper demMapper16

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin
vtkInteractorStyleTerrain t
iren SetInteractorStyle t

# Add the actors to the renderer, set the background and size
#
ren1 AddActor demActor
ren1 SetBackground .4 .4 .4

iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren SetDesiredUpdateRate 1

wm withdraw .
proc TkCheckAbort {} {
  set foo [renWin GetEventPending]
    if {$foo != 0} {renWin SetAbortRender 1}
}
renWin AddObserver AbortCheckEvent {TkCheckAbort}

[ren1 GetActiveCamera] SetViewUp 0 0 1
[ren1 GetActiveCamera] SetPosition -99900 -21354 131801
[ren1 GetActiveCamera] SetFocalPoint 41461 41461 2815
ren1 ResetCamera
[ren1 GetActiveCamera] Dolly 1.2
ren1 ResetCameraClippingRange

renWin Render
