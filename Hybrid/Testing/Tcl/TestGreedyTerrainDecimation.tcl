package require vtk
package require vtkinteraction

vtkLookupTable lut
  lut SetHueRange 0.6 0
  lut SetSaturationRange 1.0 0
  lut SetValueRange 0.5 1.0

# Read the data: a height field results
vtkDEMReader demReader
  demReader SetFileName $VTK_DATA_ROOT/Data/SainteHelens.dem
  demReader Update

set lo [lindex [[demReader GetOutput] GetScalarRange] 0]
set hi [lindex [[demReader GetOutput] GetScalarRange] 1]

# Decimate the terrain
vtkGreedyTerrainDecimation deci
  deci SetInput [demReader GetOutput]
  deci BoundaryVertexDeletionOn
#  deci SetErrorMeasureToSpecifiedReduction
#  deci SetReduction 0.95
  deci SetErrorMeasureToNumberOfTriangles
  deci SetNumberOfTriangles 5000
#  deci SetErrorMeasureToAbsoluteError
#  deci SetAbsoluteError 25.0
#  deci SetErrorMeasureToRelativeError
#  deci SetAbsoluteError 0.01

vtkPolyDataNormals normals
  normals SetInput [deci GetOutput]
  normals SetFeatureAngle 60
  normals ConsistencyOn
  normals SplittingOff

vtkPolyDataMapper demMapper
  demMapper SetInput [normals GetOutput]
  eval demMapper SetScalarRange $lo $hi
  demMapper SetLookupTable lut

vtkLODActor actor
  actor SetMapper demMapper
  
# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor actor
ren1 SetBackground .1 .2 .4

iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren SetDesiredUpdateRate 5

[ren1 GetActiveCamera] SetViewUp 0 0 1
[ren1 GetActiveCamera] SetPosition -99900 -21354 131801
[ren1 GetActiveCamera] SetFocalPoint 41461 41461 2815
ren1 ResetCamera
[ren1 GetActiveCamera] Dolly 1.2
ren1 ResetCameraClippingRange

renWin Render

wm withdraw .
