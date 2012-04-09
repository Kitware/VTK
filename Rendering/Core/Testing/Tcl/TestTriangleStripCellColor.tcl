package require vtk
package require vtkinteraction

vtkSphereSource sphere 

vtkElevationFilter elevation
elevation SetInputConnection [sphere GetOutputPort]
elevation SetLowPoint -1 0 0
elevation SetHighPoint 1 0 0

vtkPointDataToCellData p2c
p2c SetInputConnection [elevation GetOutputPort]

vtkStripper stripper
stripper SetInputConnection [p2c GetOutputPort]
stripper PassCellDataAsFieldDataOn

vtkPolyDataMapper sphereMapper
sphereMapper SetInputConnection [stripper GetOutputPort]
sphereMapper SelectColorArray "Elevation"
sphereMapper SetColorModeToMapScalars
sphereMapper SetScalarModeToUseFieldData
sphereMapper SetScalarRange 0.28 0.72 

vtkActor sphereActor
sphereActor SetMapper sphereMapper
#interpolation must be set to flat for cell colors to work
#for triangle strips.
[sphereActor GetProperty] SetInterpolationToFlat

vtkRenderer ren1
ren1 AddActor sphereActor

vtkRenderWindow renWin
renWin AddRenderer ren1
renWin SetSize 200 200

vtkRenderWindowInteractor iren
iren SetRenderWindow renWin

iren AddObserver UserEvent {wm deiconify .vtkInteract}
renWin Render
wm withdraw .
