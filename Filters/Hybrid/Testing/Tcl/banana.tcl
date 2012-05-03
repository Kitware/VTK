package require vtk
package require vtkinteraction


# use a sphere as a basis of the shape
vtkSphereSource sphere
sphere SetPhiResolution 40
sphere SetThetaResolution 40
sphere Update

set sphereData [sphere GetOutput]

# create a data array to hold the weighting coefficients
vtkFloatArray tfarray
set npoints [$sphereData GetNumberOfPoints]
tfarray SetNumberOfComponents 2
tfarray SetNumberOfTuples $npoints

# parameterize the sphere along the z axis, and fill the weights
# with (1.0-a, a) to linearly interpolate across the shape
for {set i 0} {$i < $npoints} {incr i} {
    set pt [$sphereData GetPoint $i]
    set x [lindex $pt 0]
    set y [lindex $pt 1]
    set z [lindex $pt 2]
#foreach {x y z} $pt {}

    # -0.5 < z < 0.5
    set zn [expr $z + 0.5]
    set zn1 [expr 1.0 - $zn]
    if {$zn > 1.0} {set zn 1.0}
    if {$zn1 < 0.0} {set zn1 0.0}

    tfarray SetComponent $i 0 $zn1
    tfarray SetComponent $i 1 $zn
}

# create field data to hold the array, and bind it to the sphere
vtkFieldData fd
tfarray SetName "weights"
[$sphereData GetPointData] AddArray tfarray

# use an ordinary transform to stretch the shape
vtkTransform stretch
stretch Scale 1 1 3.2

vtkTransformFilter stretchFilter
stretchFilter SetInputData $sphereData
stretchFilter SetTransform stretch

# now, for the weighted transform stuff
vtkWeightedTransformFilter weightedTrans

# create two transforms to interpolate between
vtkTransform identity
identity Identity

vtkTransform rotated
set rotatedAngle 45
rotated RotateX $rotatedAngle

weightedTrans SetNumberOfTransforms 2
weightedTrans SetTransform identity  0
weightedTrans SetTransform rotated   1
# which data array should the filter use ?
weightedTrans SetWeightArray "weights"

weightedTrans SetInputConnection [stretchFilter GetOutputPort]

vtkPolyDataMapper weightedTransMapper
    weightedTransMapper SetInputConnection [weightedTrans GetOutputPort]
vtkActor weightedTransActor
    weightedTransActor SetMapper weightedTransMapper
    [weightedTransActor GetProperty] SetDiffuseColor 0.8 0.8 0.1
    [weightedTransActor GetProperty] SetRepresentationToSurface


# create simple poly data so we can apply glyph

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
renWin SetSize 300 300

ren1 ResetCamera
[ren1 GetActiveCamera] Azimuth 90
[ren1 GetActiveCamera] Dolly 1
# Get handles to some useful objects
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}
renWin Render

# prevent the tk window from showing up then start the event loop

proc cmd {s} {
    rotated Identity
    rotated RotateX $s
    renWin Render
}
cmd $rotatedAngle

wm withdraw .
