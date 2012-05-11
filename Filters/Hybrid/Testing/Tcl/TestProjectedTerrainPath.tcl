package require vtk
package require vtkinteraction

vtkLookupTable lut
  lut SetHueRange 0.6 0
  lut SetSaturationRange 1.0 0
  lut SetValueRange 0.5 1.0

# Read the data: a height field results
vtkDEMReader demReader
  demReader SetFileName "$VTK_DATA_ROOT/Data/SainteHelens.dem"
  demReader Update

set lo [lindex [[demReader GetOutput] GetScalarRange] 0]
set hi [lindex [[demReader GetOutput] GetScalarRange] 1]

vtkImageDataGeometryFilter surface
  surface SetInputConnection [demReader GetOutputPort]

vtkWarpScalar warp
  warp SetInputConnection [surface GetOutputPort]
  warp SetScaleFactor 1
  warp UseNormalOn
  warp SetNormal 0 0 1

vtkPolyDataNormals normals
  normals SetInputConnection [warp GetOutputPort]
  normals SetFeatureAngle 60
  normals SplittingOff

vtkPolyDataMapper demMapper
  demMapper SetInputConnection [normals GetOutputPort]
  eval demMapper SetScalarRange $lo $hi
  demMapper SetLookupTable lut

vtkLODActor demActor
  demActor SetMapper demMapper

# Create some paths
vtkPoints pts
  pts InsertNextPoint 562669 5.1198e+006 1992.77
  pts InsertNextPoint 562801 5.11618e+006 2534.97
  pts InsertNextPoint 562913 5.11157e+006 1911.1
  pts InsertNextPoint 559849 5.11083e+006 1681.34
  pts InsertNextPoint 562471 5.11633e+006 2593.57
  pts InsertNextPoint 563223 5.11616e+006 2598.31
  pts InsertNextPoint 566579 5.11127e+006 1697.83
  pts InsertNextPoint 569000 5.11127e+006 1697.83
vtkCellArray lines
  lines InsertNextCell 3
  lines InsertCellPoint 0
  lines InsertCellPoint 1
  lines InsertCellPoint 2
  lines InsertNextCell 5
  lines InsertCellPoint 3
  lines InsertCellPoint 4
  lines InsertCellPoint 5
  lines InsertCellPoint 6
  lines InsertCellPoint 7

vtkPolyData terrainPaths
  terrainPaths SetPoints pts
  terrainPaths SetLines lines

vtkProjectedTerrainPath projectedPaths
  projectedPaths SetInputData terrainPaths
  projectedPaths SetSourceConnection [demReader GetOutputPort]
  projectedPaths SetHeightOffset 25
  projectedPaths SetHeightTolerance 5
  projectedPaths SetProjectionModeToNonOccluded
  projectedPaths SetProjectionModeToHug

vtkPolyDataMapper pathMapper
  pathMapper SetInputConnection [projectedPaths GetOutputPort]

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
