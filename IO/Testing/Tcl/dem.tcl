package require vtk
package require vtkinteraction

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

set Scale 5
vtkLookupTable lut
  lut SetHueRange 0.6 0
  lut SetSaturationRange 1.0 0
  lut SetValueRange 0.5 1.0

vtkDEMReader demModel
  demModel SetFileName $VTK_DATA_ROOT/Data/SainteHelens.dem
  demModel Update

demModel Print

set lo [expr $Scale * [lindex [demModel GetElevationBounds] 0]]
set hi [expr $Scale * [lindex [demModel GetElevationBounds] 1]]

vtkLODActor demActor

# create a pipeline for each lod mapper
set lods {4 8 16}
foreach lod $lods {
  vtkImageShrink3D shrink$lod
    shrink$lod SetShrinkFactors $lod $lod 1
    shrink$lod SetInput [demModel GetOutput]
    shrink$lod AveragingOn

  vtkImageDataGeometryFilter geom$lod
    geom$lod SetInput [shrink$lod GetOutput]
    geom$lod ReleaseDataFlagOn

  vtkWarpScalar warp$lod
    warp$lod SetInput [geom$lod GetOutput]
    warp$lod SetNormal 0 0 1
    warp$lod UseNormalOn
    warp$lod SetScaleFactor $Scale
    warp$lod ReleaseDataFlagOn

  vtkElevationFilter elevation$lod
    elevation$lod SetInput [warp$lod GetOutput]
    elevation$lod SetLowPoint 0 0 $lo
    elevation$lod SetHighPoint 0 0 $hi
eval elevation$lod SetScalarRange $lo $hi
    elevation$lod ReleaseDataFlagOn

  vtkCastToConcrete toPoly$lod
    toPoly$lod SetInput [elevation$lod GetOutput]
  
  vtkPolyDataNormals normals$lod
    normals$lod SetInput [toPoly$lod GetPolyDataOutput]
    normals$lod SetFeatureAngle 60
    normals$lod ConsistencyOff
    normals$lod SplittingOff
    normals$lod ReleaseDataFlagOn

  vtkPolyDataMapper demMapper$lod
    demMapper$lod SetInput [normals$lod GetOutput]
eval demMapper$lod SetScalarRange $lo $hi
    demMapper$lod SetLookupTable lut
    demMapper$lod ImmediateModeRenderingOn

demMapper$lod Update

demActor AddLODMapper demMapper$lod
}

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
