catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# this is a tcl version: tests weighted transform filter
# include the vtk interactor ui
source $VTK_TCL/vtkInt.tcl

vtkSphereSource sphere
sphere SetPhiResolution 40
sphere SetThetaResolution 40
sphere Update

set sphereData [sphere GetOutput]

vtkFloatArray tfarray
set npoints [$sphereData GetNumberOfPoints]
tfarray SetNumberOfComponents 2
tfarray SetNumberOfTuples $npoints

# parameterize a sphere along the z axis
for {set i 0} {$i < $npoints} {incr i} {
    set pt [$sphereData GetPoint $i]
    foreach {x y z} $pt {}

    # -0.5 < z < 0.5
    set zn [expr {$z + 0.5}]
    set zn1 [expr {1.0 - $zn}]
    if {$zn > 1.0} {set zn 1.0}
    if {$zn1 < 0.0} {set zn1 0.0}

    tfarray SetComponent $i 0 $zn1
    tfarray SetComponent $i 1 $zn
}

vtkFieldData fd
fd AddArray tfarray "weights"
[$sphereData GetPointData] SetFieldData fd

vtkTransform stretch
stretch Scale 1 1 3.2

vtkTransformFilter stretchFilter
stretchFilter SetInput $sphereData
stretchFilter SetTransform stretch

vtkWeightedTransformFilter weightedTrans

vtkTransform identity
identity Identity

set rotatedAngle 60

# a perspective transform tests the generalized transform path in the code
vtkPerspectiveTransform rotated
rotated RotateX $rotatedAngle

weightedTrans SetNumberOfTransforms 2
weightedTrans SetTransform identity  0
weightedTrans SetTransform rotated   1
weightedTrans SetWeightArray "weights"

weightedTrans SetInput [stretchFilter GetOutput]

vtkPolyDataMapper weightedTransMapper
    weightedTransMapper SetInput [weightedTrans GetOutput]
vtkActor weightedTransActor
    weightedTransActor SetMapper weightedTransMapper
    [weightedTransActor GetProperty] SetDiffuseColor 0.8 0.8 0.1

#
# Create the rendering stuff
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

ren1 AddActor weightedTransActor
ren1 SetBackground 0.1 0.2 0.5
renWin SetSize 450 450

[ren1 GetActiveCamera] Azimuth 90
[ren1 GetActiveCamera] Dolly 0.85
# Get handles to some useful objects
#
iren SetUserMethod {wm deiconify .vtkInteract}
renWin Render

# prevent the tk window from showing up then start the event loop

#renWin SetFileName TestWeightedTransformFilter.tcl.ppm
#renWin SaveImageAsPPM

wm withdraw .