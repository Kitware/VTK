catch {load vtktcl}
# this is a tcl version: tests cell neighbors
# include get the vtk interactor ui
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }
source $VTK_TCL/vtkInt.tcl

vtkLookupTable layers
   layers SetNumberOfColors   3
   layers Build
   layers SetTableValue 0 0.4 1.0 0.4 1.0
   layers SetTableValue 1 1.0 0.3 0.3 1.0
   layers SetTableValue 2 1.0 0.3 0.3 1.0

vtkSphereSource sphere
   sphere SetPhiResolution   20
   sphere SetThetaResolution 20

vtkTriangleFilter triangles
   triangles SetInput [sphere GetOutput]
   triangles Update

vtkScalars neighbors
   neighbors SetNumberOfScalars [[triangles GetOutput] GetNumberOfCells]

for {set i 0} {$i < [[triangles GetOutput] GetNumberOfCells]} {incr i} {
   neighbors SetScalar $i 0.0
}

vtkIdList PointList
vtkIdList CellList

[triangles GetOutput] GetCellPoints    645 PointList
[triangles GetOutput] GetCellNeighbors 645 PointList CellList
for {set i 0} {$i < [CellList GetNumberOfIds]} {incr i} {
    neighbors SetScalar [CellList GetId $i] 1.0
}

[triangles GetOutput] GetCellPoints    670 PointList
[triangles GetOutput] GetCellNeighbors 670 PointList CellList
for {set i 0} {$i < [CellList GetNumberOfIds]} {incr i} {
    neighbors SetScalar [CellList GetId $i] 1.0
}

[triangles GetOutput] GetCellPoints    505 PointList
[triangles GetOutput] GetCellNeighbors 505 PointList CellList
for {set i 0} {$i < [CellList GetNumberOfIds]} {incr i} {
    neighbors SetScalar [CellList GetId $i] 1.0
}

[[triangles GetOutput] GetCellData] SetScalars neighbors

vtkShrinkPolyData shrink
   shrink SetInput [triangles GetOutput]
   shrink SetShrinkFactor .8

vtkPolyDataMapper map
   map SetInput [shrink GetOutput]
   map SetLookupTable layers
   map ScalarVisibilityOn
   map SetScalarRange 0.0 1.0
   map SetScalarModeToUseCellData

vtkActor sphereActor
   sphereActor SetMapper map

#
# Create the rendering stuff
#
vtkRenderer ren1
  ren1 TwoSidedLightingOff

vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

ren1 AddActor sphereActor
ren1 SetBackground 0.1 0.2 0.4
renWin SetSize  300 300
[ren1 GetActiveCamera] Dolly 1.5
ren1 ResetCameraClippingRange

[ren1 GetActiveCamera] SetPosition   0.97 -1.85 -0.97
[ren1 GetActiveCamera] SetFocalPoint 0.0 0.0 0.0 
[ren1 GetActiveCamera] SetViewAngle  30
[ren1 GetActiveCamera] SetViewUp     0.58 -0.11 0.80

# Get handles to some useful objects
#
iren SetUserMethod {wm deiconify .vtkInteract}
renWin Render
#renWin SetFileName "valid/CellNeighbors.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .


