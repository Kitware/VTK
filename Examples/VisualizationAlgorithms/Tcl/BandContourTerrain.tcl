# In this example we show the use of the vtkBandedPolyDataContourFilter.
# This filter creates separate, constant colored bands for a range of scalar
# values. Each band is bounded by two scalar values, and the cell data lying
# within the value has the same cell scalar value.

package require vtk
package require vtkinteraction
package require vtktesting

# The lookup table is similar to that used by maps. Two hues are used: a
# brown for land, and a blue for water. The value of the hue is changed to
# give the effect of elevation.
set Scale 5 
vtkLookupTable lutWater
  lutWater SetNumberOfColors 10
  lutWater SetHueRange 0.58 0.58
  lutWater SetSaturationRange 0.5 0.1
  lutWater SetValueRange 0.5 1.0
  lutWater Build
vtkLookupTable lutLand
  lutLand SetNumberOfColors 10
  lutLand SetHueRange 0.1 0.1
  lutLand SetSaturationRange 0.4 0.1
  lutLand SetValueRange 0.55 0.9
  lutLand Build


# The DEM reader reads data and creates an output image.
vtkDEMReader demModel
  demModel SetFileName $VTK_DATA_ROOT/Data/SainteHelens.dem
  demModel Update

# We shrink the terrain data down a bit to yield better performance for 
# this example.
set shrinkFactor 4
vtkImageShrink3D shrink
  shrink SetShrinkFactors $shrinkFactor $shrinkFactor 1
  shrink SetInputConnection [demModel GetOutputPort]
  shrink AveragingOn

# Convert the image into polygons.
vtkImageDataGeometryFilter geom
  geom SetInputConnection [shrink GetOutputPort]

# Warp the polygons based on elevation.
vtkWarpScalar warp
  warp SetInputConnection [geom GetOutputPort]
  warp SetNormal 0 0 1
  warp UseNormalOn
  warp SetScaleFactor $Scale

# Create the contour bands.
vtkBandedPolyDataContourFilter bcf
  bcf SetInput [warp GetPolyDataOutput]
  eval bcf GenerateValues 15 [[demModel GetOutput] GetScalarRange]
  bcf SetScalarModeToIndex
  bcf GenerateContourEdgesOn

# Compute normals to give a better look.
vtkPolyDataNormals normals
  normals SetInputConnection [bcf GetOutputPort]
  normals SetFeatureAngle 60
  normals ConsistencyOff
  normals SplittingOff

vtkPolyDataMapper demMapper
  demMapper SetInputConnection [normals GetOutputPort]
  eval demMapper SetScalarRange 0 10
  demMapper SetLookupTable lutLand
  demMapper SetScalarModeToUseCellData

vtkLODActor demActor
  demActor SetMapper demMapper

## Create contour edges
vtkPolyDataMapper edgeMapper
  edgeMapper SetInput [bcf GetContourEdgesOutput]
  edgeMapper SetResolveCoincidentTopologyToPolygonOffset 
vtkActor edgeActor
  edgeActor SetMapper edgeMapper
  [edgeActor GetProperty] SetColor 0 0 0

## Test clipping
# Create the contour bands.
vtkBandedPolyDataContourFilter bcf2
  bcf2 SetInput [warp GetPolyDataOutput]
  bcf2 ClippingOn
  eval bcf2 GenerateValues 10 1000 2000
  bcf2 SetScalarModeToValue

# Compute normals to give a better look.
vtkPolyDataNormals normals2
  normals2 SetInputConnection [bcf2 GetOutputPort]
  normals2 SetFeatureAngle 60
  normals2 ConsistencyOff
  normals2 SplittingOff

vtkLookupTable lut
lut SetNumberOfColors 10
vtkPolyDataMapper demMapper2
  demMapper2 SetInputConnection [normals2 GetOutputPort]
  eval demMapper2 SetScalarRange [[demModel GetOutput] GetScalarRange]
  demMapper2 SetLookupTable lut
  demMapper2 SetScalarModeToUseCellData

vtkLODActor demActor2
  demActor2 SetMapper demMapper2
  demActor2 AddPosition 0 15000 0

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor demActor
ren1 AddActor demActor2
ren1 AddActor edgeActor

ren1 SetBackground .4 .4 .4
renWin SetSize 375 200

vtkCamera cam
  cam SetPosition -17438.8 2410.62 25470.8
  cam SetFocalPoint 3985.35 11930.6 5922.14
  cam SetViewUp 0 0 1
ren1 SetActiveCamera cam
ren1 ResetCamera
cam Zoom 2
 
iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren SetDesiredUpdateRate 1

proc TkCheckAbort {} {
  set foo [renWin GetEventPending]
    if {$foo != 0} {renWin SetAbortRender 1}
}
renWin AddObserver AbortCheckEvent {TkCheckAbort}
renWin Render

wm withdraw .
