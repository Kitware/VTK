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

vtkGeometryFilter surface
  surface SetInput [demReader GetOutput]

vtkWarpScalar warp
  warp SetInput [surface GetOutput]
  warp SetScaleFactor 1
  warp UseNormalOn
  warp SetNormal 0 0 1

vtkPolyDataNormals normals
  normals SetInput [warp GetPolyDataOutput]
  normals SetFeatureAngle 60
  normals SplittingOff

vtkPolyDataMapper demMapper
  demMapper SetInput [normals GetOutput]
  eval demMapper SetScalarRange $lo $hi
  demMapper SetLookupTable lut

vtkLODActor demActor
  demActor SetMapper demMapper
  
# Create some paths
vtkPoints pts
  pts InsertNextPoint 562669 5.1198e+006 1992.77
  pts InsertNextPoint 562801 5.11618e+006 2534.97
  pts InsertNextPoint 562913 5.11157e+006 1911.1
vtkCellArray lines
  lines InsertNextCell 3
  lines InsertCellPoint 0
  lines InsertCellPoint 1
  lines InsertCellPoint 2
vtkPolyData terrainPaths
  terrainPaths SetPoints pts
  terrainPaths SetLines lines

vtkProjectedTerrainPath projectedPaths
  projectedPaths SetInput terrainPaths
  projectedPaths SetSource [demReader GetOutput]

vtkPolyDataMapper pathMapper
  pathMapper SetInput [projectedPaths GetOutput]

vtkActor paths
  paths SetMapper pathMapper
  [paths GetProperty] SetColor 1 0 0

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
ren1 AddActor paths
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
