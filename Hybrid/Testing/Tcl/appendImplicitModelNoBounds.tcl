package require vtk
package require vtkinteraction

# this demonstrates appending data to generate an implicit model
# contrast this with appendImplicitModel.tcl which set the bounds
# explicitly. this scrip should produce the same results.


vtkCubeSource cubeForBounds
  cubeForBounds SetBounds -2.5 2.5 -2.5 2.5 -2.5 2.5
  cubeForBounds Update

vtkLineSource lineX
    lineX SetPoint1 -2.0 0.0 0.0
    lineX SetPoint2  2.0 0.0 0.0
    lineX Update
vtkLineSource lineY
    lineY SetPoint1  0.0 -2.0 0.0
    lineY SetPoint2  0.0  2.0 0.0
    lineY Update
vtkLineSource lineZ
    lineZ SetPoint1 0.0 0.0 -2.0
    lineZ SetPoint2 0.0 0.0  2.0
    lineZ Update
vtkPlaneSource aPlane
    aPlane Update

# set Data(3) "lineX"
# set Data(1) "lineY"
# set Data(2) "lineZ"
# set Data(0) "aPlane"

vtkImplicitModeller imp
    imp SetSampleDimensions 60 60 60
    imp SetCapValue 1000
    imp ComputeModelBounds [cubeForBounds GetOutput]

# Okay now let's see if we can append
imp StartAppend
# for {set i 0} {$i < 4} {incr i} {
#     imp Append [$Data($i) GetOutput]
# }
imp Append [aPlane GetOutput]
imp Append [lineZ GetOutput]
imp Append [lineY GetOutput]
imp Append [lineX GetOutput]
imp EndAppend


vtkContourFilter cf
cf SetInputConnection [imp GetOutputPort]
    cf SetValue 0 0.1
vtkPolyDataMapper mapper
    mapper SetInputConnection [cf GetOutputPort]
vtkActor actor
    actor SetMapper mapper

vtkOutlineFilter outline
    outline SetInputConnection [imp GetOutputPort]
vtkPolyDataMapper outlineMapper
    outlineMapper SetInputConnection [outline GetOutputPort]
vtkActor outlineActor
    outlineActor SetMapper outlineMapper

vtkImageDataGeometryFilter plane
    plane SetInputConnection [imp GetOutputPort]
    plane SetExtent 0 60 0 60 30 30
vtkPolyDataMapper planeMapper
    planeMapper SetInputConnection [plane GetOutputPort]
    planeMapper SetScalarRange 0.197813 0.710419
vtkActor planeActor
    planeActor SetMapper planeMapper

# graphics stuff
vtkRenderer ren1
    ren1 AddActor actor
    ren1 AddActor planeActor
    ren1 AddActor outlineActor
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

ren1 SetBackground 0.1 0.2 0.4
iren AddObserver UserEvent {wm deiconify .vtkInteract}

renWin Render

[ren1 GetActiveCamera] Azimuth 30
[ren1 GetActiveCamera] Elevation 30
ren1 ResetCameraClippingRange
renWin Render


wm withdraw .
