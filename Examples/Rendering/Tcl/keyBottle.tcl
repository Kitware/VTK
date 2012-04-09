# This example demonstrates keyframe animation. It changes the
# camera's azimuth by interpolating given azimuth values (using splines)

package require vtk

# Get keyframe procs
# See KeyFrame.tcl for details.
source KeyFrame.tcl

# Reduce the number of frames if this example
# is taking too long
set NumberOfFrames 1200

# Create bottle profile. This is the object to be rendered.
vtkPoints points
points InsertPoint 0 0.01 0.0 0.0
points InsertPoint 1 1.5 0.0 0.0
points InsertPoint 2 1.5 0.0 3.5
points InsertPoint 3 1.25 0.0 3.75
points InsertPoint 4 0.75 0.0 4.00
points InsertPoint 5 0.6 0.0 4.35
points InsertPoint 6 0.7 0.0 4.65
points InsertPoint 7 1.0 0.0 4.75
points InsertPoint 8 1.0 0.0 5.0
points InsertPoint 9 0.01 0.0 5.0

vtkCellArray lines
lines InsertNextCell 10;#number of points
lines InsertCellPoint 0
lines InsertCellPoint 1
lines InsertCellPoint 2
lines InsertCellPoint 3
lines InsertCellPoint 4
lines InsertCellPoint 5
lines InsertCellPoint 6
lines InsertCellPoint 7
lines InsertCellPoint 8
lines InsertCellPoint 9

vtkPolyData profile
profile SetPoints points
profile SetLines lines

# Extrude profile to make bottle
vtkRotationalExtrusionFilter extrude
extrude SetInputData profile
extrude SetResolution 60

vtkPolyDataMapper map
map SetInputConnection [extrude GetOutputPort]

vtkActor bottle
bottle SetMapper map
[bottle GetProperty] SetColor 0.3800 0.7000 0.1600

# Create the RenderWindow, Renderer
vtkRenderer ren1
vtkRenderWindow renWin
renWin AddRenderer ren1

# Add the actor to the renderer, set the background and size#
ren1 AddActor bottle
ren1 SetBackground 1 1 1
renWin SetSize 500 500
# First render, forces the renderer to create a camera with a
# good initial position
renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .

# Initialize keyframe recording by passing the camera object
# and the method used to change the position
set camera [ren1 GetActiveCamera]
KeyNew Azimuth $camera SetPosition

# Define the key frames.
# This is done by changing the position of the camera with
# $camera Azimuth and recording it with KeyAdd
# This is far simpler than calculating the new position by hand.
KeyAdd Azimuth [$camera GetPosition]
$camera Azimuth 1
KeyAdd Azimuth [$camera GetPosition]
$camera Azimuth 2
KeyAdd Azimuth [$camera GetPosition]
for {set i 0} {$i <= 36} {incr i} {
  $camera Azimuth 10
  KeyAdd Azimuth [$camera GetPosition]
}
$camera Azimuth 2
KeyAdd Azimuth [$camera GetPosition]
$camera Azimuth 1
KeyAdd Azimuth [$camera GetPosition]

$camera Azimuth 0
KeyAdd Azimuth [$camera GetPosition]

$camera Azimuth -1
KeyAdd Azimuth [$camera GetPosition]
$camera Azimuth -2
KeyAdd Azimuth [$camera GetPosition]
for {set i 0} {$i <= 36} {incr i} {
  $camera Azimuth -10
  KeyAdd Azimuth [$camera GetPosition]
}
$camera Azimuth -2
KeyAdd Azimuth [$camera GetPosition]
$camera Azimuth -1
KeyAdd Azimuth [$camera GetPosition]

# Run the animation - NumberOfFrames frames -
# using interpolation
KeyRun Azimuth $NumberOfFrames

# Clean-up and exit
vtkCommand DeleteAllObjects
exit



