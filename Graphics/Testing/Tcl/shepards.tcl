package require vtk
package require vtkinteraction

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# create some points
#
vtkMath math
vtkPoints points
for {set i 0} {$i<50} {incr i 1} {
    eval points InsertPoint $i [math Random 0 1] [math Random 0 1] [math Random 0 1]
}

vtkFloatArray scalars
for {set i 0} {$i<50} {incr i 1} {
    eval scalars InsertValue $i [math Random 0 1]
}

vtkPolyData profile
    profile SetPoints points
    [profile GetPointData] SetScalars scalars

# triangulate them
#
vtkShepardMethod shepard
    shepard SetInput profile
    shepard SetModelBounds 0 1 0 1 .1 .5
#    shepard SetMaximumDistance .1
    shepard SetNullValue 1
    shepard SetSampleDimensions 20 20 20
    shepard Update
    
vtkDataSetMapper map
    map SetInput [shepard GetOutput]

vtkActor block
    block SetMapper map
    [block GetProperty] SetColor 1 0 0

# Add the actors to the renderer, set the background and size
#
ren1 AddActor block
ren1 SetBackground 1 1 1
renWin SetSize 400 400

set cam1 [ren1 GetActiveCamera]
$cam1 Azimuth 160
$cam1 Elevation 30
$cam1 Zoom 1.5
ren1 ResetCameraClippingRange

renWin Render

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .

set threshold 15

