package require vtk
package require vtkinteraction
package require vtktesting

# This example demonstrates how to use the vtkSphereWidget to control the
# position of a light.

# Start by loading some data.
#
vtkDEMReader dem
    dem SetFileName "$VTK_DATA_ROOT/Data/SainteHelens.dem"
    dem Update

set Scale 2
vtkLookupTable lut
  lut SetHueRange 0.6 0
  lut SetSaturationRange 1.0 0
  lut SetValueRange 0.5 1.0
set lo [expr $Scale * [lindex [dem GetElevationBounds] 0]]
set hi [expr $Scale * [lindex [dem GetElevationBounds] 1]]

vtkImageShrink3D shrink
  shrink SetShrinkFactors 4 4 1
  shrink SetInputConnection [dem GetOutputPort]
  shrink AveragingOn

vtkImageDataGeometryFilter geom
  geom SetInputConnection [shrink GetOutputPort]
  geom ReleaseDataFlagOn

vtkWarpScalar warp
  warp SetInputConnection [geom GetOutputPort]
  warp SetNormal 0 0 1
  warp UseNormalOn
  warp SetScaleFactor $Scale
  warp ReleaseDataFlagOn

vtkElevationFilter elevation
  elevation SetInputConnection [warp GetOutputPort]
  elevation SetLowPoint 0 0 $lo
  elevation SetHighPoint 0 0 $hi
  eval elevation SetScalarRange $lo $hi
  elevation ReleaseDataFlagOn

vtkPolyDataNormals normals
  normals SetInputConnection [elevation GetOutputPort]
  normals SetFeatureAngle 60
  normals ConsistencyOff
  normals SplittingOff
  normals ReleaseDataFlagOn

vtkPolyDataMapper demMapper
  demMapper SetInputConnection [normals GetOutputPort]
  eval demMapper SetScalarRange $lo $hi
  demMapper SetLookupTable lut
  demMapper ImmediateModeRenderingOn

vtkLODActor demActor
  demActor SetMapper demMapper

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin
    iren LightFollowCameraOff
    iren SetInteractorStyle ""

# Associate the line widget with the interactor
vtkSphereWidget sphereWidget
sphereWidget SetInteractor iren
sphereWidget SetProp3D demActor
sphereWidget SetPlaceFactor 4
sphereWidget PlaceWidget
sphereWidget TranslationOff
sphereWidget ScaleOff
sphereWidget HandleVisibilityOn
sphereWidget AddObserver InteractionEvent MoveLight

# Add the actors to the renderer, set the background and size
#
ren1 AddActor demActor
ren1 SetBackground 1 1 1
renWin SetSize 300 300
ren1 SetBackground 0.1 0.2 0.4

set cam1 [ren1 GetActiveCamera]
$cam1 SetViewUp 0 0 1
eval $cam1 SetFocalPoint [[dem GetOutput] GetCenter]
$cam1 SetPosition 1 0 0
ren1 ResetCamera
$cam1 Elevation 25
$cam1 Azimuth 125
$cam1 Zoom 1.25

vtkLight light
eval light SetFocalPoint [[dem GetOutput] GetCenter]
ren1 AddLight light

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}
renWin Render

# Prevent the tk window from showing up then start the event loop.
wm withdraw .

# Actually probe the data

proc MoveLight {} {
    eval light SetPosition [sphereWidget GetHandlePosition]
}

