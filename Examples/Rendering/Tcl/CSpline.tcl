# This example demonstrates the use of vtkCardinalSpline.
# It creates random points and connects them with a spline

#
# First we include the VTK Tcl packages which will make available 
# all of the vtk commands to Tcl
#
package require vtk
package require vtkinteraction
package require vtktesting

# This will be used later to get random numbers.
vtkMath math

# Total number of points.
set numberOfInputPoints 10

# One spline for each direction.
vtkCardinalSpline aSplineX
vtkCardinalSpline aSplineY
vtkCardinalSpline aSplineZ

# Generate random (pivot) points and add the corresponding 
# coordinates to the splines.
# aSplineX will interpolate the x values of the points
# aSplineY will interpolate the y values of the points
# aSplineZ will interpolate the z values of the points
vtkPoints inputPoints
for {set i 0} {$i < $numberOfInputPoints} {incr i 1} {
    set x  [math Random 0 1]
    set y  [math Random 0 1]
    set z  [math Random 0 1]
    aSplineX AddPoint $i $x
    aSplineY AddPoint $i $y
    aSplineZ AddPoint $i $z
    inputPoints InsertPoint $i $x $y $z
}

# The following section will create glyphs for the pivot points
# in order to make the effect of the spline more clear.

# Create a polydata to be glyphed.
vtkPolyData inputData
inputData SetPoints inputPoints

# Use sphere as glyph source.
vtkSphereSource balls
balls SetRadius .01
balls SetPhiResolution 10
balls SetThetaResolution 10

vtkGlyph3D glyphPoints
glyphPoints SetInput inputData
glyphPoints SetSource [balls GetOutput]

vtkPolyDataMapper glyphMapper
glyphMapper SetInput [glyphPoints GetOutput]

vtkActor glyph
glyph SetMapper glyphMapper
eval   [glyph GetProperty] SetDiffuseColor $tomato
[glyph GetProperty] SetSpecular .3
[glyph GetProperty] SetSpecularPower 30

# Generate the polyline for the spline.
vtkPoints points
vtkPolyData profileData

# Number of points on the spline
set numberOfOutputPoints 400

# Interpolate x, y and z by using the three spline filters and
# create new points
for {set i 0} {$i< $numberOfOutputPoints} {incr i 1} {
    set t [expr ( $numberOfInputPoints - 1.0 ) / ( $numberOfOutputPoints - 1.0 ) * $i]
    points InsertPoint $i [aSplineX Evaluate $t] [aSplineY Evaluate $t] [aSplineZ Evaluate $t]
}

# Create the polyline.
vtkCellArray lines
lines InsertNextCell $numberOfOutputPoints
for {set i 0} {$i < $numberOfOutputPoints} {incr i 1} {
  lines InsertCellPoint $i
}
profileData SetPoints points
profileData SetLines lines

# Add thickness to the resulting line.
vtkTubeFilter profileTubes
profileTubes SetNumberOfSides 8
profileTubes SetInput profileData
profileTubes SetRadius .005

vtkPolyDataMapper profileMapper
profileMapper SetInput [profileTubes GetOutput]

vtkActor profile
profile SetMapper profileMapper
eval  [profile GetProperty] SetDiffuseColor $banana
[profile GetProperty] SetSpecular .3
[profile GetProperty] SetSpecularPower 30


# Now create the RenderWindow, Renderer and Interactor
#
vtkRenderer ren1
vtkRenderWindow renWin
renWin AddRenderer ren1

vtkRenderWindowInteractor iren
iren SetRenderWindow renWin

# Add the actors
ren1 AddActor glyph
ren1 AddActor profile

renWin SetSize 500 500

# render the image
#
iren Initialize
iren AddObserver UserEvent {wm deiconify .vtkInteract}

# prevent the tk window from showing up
wm withdraw .


